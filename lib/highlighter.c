/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_token_cursor.h"

#include "grn_pat.h"
#include "grn_str.h"

typedef struct {
  uint64_t offset;
  uint32_t length;
  grn_bool is_overlapping;
  uint8_t first_character_length;
} grn_highlighter_location;

static int
grn_highlighter_location_compare(const void *data1, const void *data2)
{
  const grn_highlighter_location *location1 = data1;
  const grn_highlighter_location *location2 = data2;

  if (location1->offset == location2->offset) {
    return location1->length - location2->length;
  } else {
    return location1->offset - location2->offset;
  }
}

/*
 * TODO:
 *   * Support non HTML mode.
 *   * Support custom tag.
 */
struct _grn_highlighter {
  grn_obj_header header;

  grn_bool is_html_mode;
  grn_bool need_prepared;
  grn_obj raw_keywords;

  struct {
    const char *open;
    size_t open_length;
    const char *close;
    size_t close_length;
  } tag;

  /* For lexicon mode */
  struct {
    grn_obj *object;
    grn_encoding encoding;
    grn_obj *token_id_chunks;
    grn_obj token_id_chunk;
    grn_obj token_ids;
    grn_obj token_locations;
    grn_obj candidates;
  } lexicon;

  /* For patricia trie mode */
  struct {
    grn_obj *keywords;
  } pat;
};

grn_highlighter *
grn_highlighter_open(grn_ctx *ctx)
{
  grn_highlighter *highlighter;

  GRN_API_ENTER;

  highlighter = GRN_MALLOCN(grn_highlighter, 1);
  if (!highlighter) {
    ERR(ctx->rc,
        "[highlighter][open] failed to allocate memory: %s",
        ctx->errbuf);
    GRN_API_RETURN(NULL);
  }

  highlighter->header.type = GRN_HIGHLIGHTER;
  highlighter->header.impl_flags = 0;
  highlighter->header.flags = 0;
  highlighter->header.domain = GRN_ID_NIL;

  highlighter->is_html_mode = GRN_TRUE;
  highlighter->need_prepared = GRN_TRUE;
  GRN_TEXT_INIT(&(highlighter->raw_keywords), GRN_OBJ_VECTOR);

  highlighter->tag.open = "<span class=\"keyword\">";
  highlighter->tag.open_length = strlen(highlighter->tag.open);
  highlighter->tag.close = "</span>";
  highlighter->tag.close_length = strlen(highlighter->tag.close);

  highlighter->lexicon.object = NULL;
  highlighter->lexicon.encoding = GRN_ENC_NONE;
  highlighter->lexicon.token_id_chunks = NULL;
  GRN_TEXT_INIT(&(highlighter->lexicon.token_id_chunk), 0);
  GRN_RECORD_INIT(&(highlighter->lexicon.token_ids), GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_TEXT_INIT(&(highlighter->lexicon.token_locations), 0);
  GRN_TEXT_INIT(&(highlighter->lexicon.candidates), 0);

  highlighter->pat.keywords = NULL;

  GRN_API_RETURN(highlighter);
}

grn_rc
grn_highlighter_close(grn_ctx *ctx,
                      grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  if (!highlighter) {
    GRN_API_RETURN(ctx->rc);
  }

  if (highlighter->pat.keywords) {
    grn_obj_close(ctx, highlighter->pat.keywords);
  }

  if (highlighter->lexicon.token_id_chunks) {
    grn_obj_close(ctx, highlighter->lexicon.token_id_chunks);
  }
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.candidates));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.token_locations));
  GRN_OBJ_FIN(ctx, &(highlighter->lexicon.token_ids));

  GRN_OBJ_FIN(ctx, &(highlighter->raw_keywords));
  GRN_FREE(highlighter);

  GRN_API_RETURN(ctx->rc);
}

static void
grn_highlighter_prepare_lexicon(grn_ctx *ctx,
                                grn_highlighter *highlighter)
{
  size_t i, n_keywords;
  grn_obj *token_id_chunk = &(highlighter->lexicon.token_id_chunk);

  highlighter->lexicon.token_ids.header.domain =
    grn_obj_id(ctx, highlighter->lexicon.object);

  grn_table_get_info(ctx,
                     highlighter->lexicon.object,
                     NULL,
                     &(highlighter->lexicon.encoding),
                     NULL,
                     NULL,
                     NULL);

  if (highlighter->lexicon.token_id_chunks) {
    grn_obj_close(ctx, highlighter->lexicon.token_id_chunks);
  }
  highlighter->lexicon.token_id_chunks =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_PAT_KEY,
                     grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                     NULL);
  if (!highlighter->lexicon.token_id_chunks) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    ERR(rc,
        "[highlighter][prepare][lexicon] "
        "failed to create an internal patricia trie: %s",
        ctx->errbuf);
    return;
  }

  n_keywords = grn_vector_size(ctx, &(highlighter->raw_keywords));
  for (i = 0; i < n_keywords; i++) {
    const char *keyword;
    unsigned int keyword_length;
    grn_token_cursor *cursor;
    grn_id token_id;

    keyword_length = grn_vector_get_element(ctx,
                                            &(highlighter->raw_keywords),
                                            i,
                                            &keyword,
                                            NULL,
                                            NULL);
    cursor = grn_token_cursor_open(ctx,
                                   highlighter->lexicon.object,
                                   keyword,
                                   keyword_length,
                                   GRN_TOKENIZE_ADD,
                                   0);
    if (!cursor) {
      continue;
    }
    while (grn_token_cursor_next(ctx, cursor) != GRN_ID_NIL) {
    }
    grn_token_cursor_close(ctx, cursor);

    cursor = grn_token_cursor_open(ctx,
                                   highlighter->lexicon.object,
                                   keyword,
                                   keyword_length,
                                   GRN_TOKENIZE_GET,
                                   0);
    if (!cursor) {
      continue;
    }
    GRN_BULK_REWIND(token_id_chunk);
    while ((token_id = grn_token_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
      GRN_TEXT_PUT(ctx, token_id_chunk, &token_id, sizeof(grn_id));
    }
    grn_token_cursor_close(ctx, cursor);
    grn_table_add(ctx,
                  highlighter->lexicon.token_id_chunks,
                  GRN_TEXT_VALUE(token_id_chunk),
                  GRN_TEXT_LEN(token_id_chunk),
                  NULL);
  }
}

static void
grn_highlighter_prepare_patricia_trie(grn_ctx *ctx,
                                      grn_highlighter *highlighter)
{
  if (highlighter->pat.keywords) {
    grn_obj_close(ctx, highlighter->pat.keywords);
  }

  highlighter->pat.keywords =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_PAT_KEY,
                     grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                     NULL);
  if (!highlighter->pat.keywords) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    ERR(rc,
        "[highlighter][prepare][no-lexicon] "
        "failed to create an internal patricia trie: %s",
        ctx->errbuf);
    return;
  }

  grn_obj_set_info(ctx,
                   highlighter->pat.keywords,
                   GRN_INFO_NORMALIZER,
                   grn_ctx_get(ctx, "NormalizerAuto", -1));

  {
    unsigned int i, n;

    n = grn_vector_size(ctx, &(highlighter->raw_keywords));
    for (i = 0; i < n; i++) {
      const char *keyword;
      unsigned int keyword_size;

      keyword_size = grn_vector_get_element(ctx,
                                            &(highlighter->raw_keywords),
                                            i,
                                            &keyword,
                                            NULL,
                                            NULL);
      grn_table_add(ctx,
                    highlighter->pat.keywords,
                    keyword,
                    keyword_size,
                    NULL);
    }
  }
}

static void
grn_highlighter_prepare(grn_ctx *ctx,
                        grn_highlighter *highlighter)
{
  if (highlighter->lexicon.object) {
    grn_highlighter_prepare_lexicon(ctx, highlighter);
  } else {
    grn_highlighter_prepare_patricia_trie(ctx, highlighter);
  }
  highlighter->need_prepared = GRN_FALSE;
}

static void
grn_highlighter_highlight_lexicon(grn_ctx *ctx,
                                  grn_highlighter *highlighter,
                                  const char *text,
                                  size_t text_length,
                                  grn_obj *output)
{
  grn_token_cursor *cursor;
  grn_encoding encoding = highlighter->lexicon.encoding;
  grn_obj *token_ids = &(highlighter->lexicon.token_ids);
  grn_obj *token_locations = &(highlighter->lexicon.token_locations);
  grn_obj *candidates = &(highlighter->lexicon.candidates);

  GRN_BULK_REWIND(token_ids);
  GRN_BULK_REWIND(token_locations);
  cursor = grn_token_cursor_open(ctx,
                                 highlighter->lexicon.object,
                                 text,
                                 text_length,
                                 GRN_TOKENIZE_ADD,
                                 0);
  if (!cursor) {
    ERR(ctx->rc,
        "[highlighter][highlight][lexicon] failed to start tokenizing: %s",
        ctx->errbuf);
    return;
  }

  while (cursor->status == GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, cursor);
    grn_highlighter_location location;
    grn_token *token;

    if (token_id == GRN_ID_NIL) {
      continue;
    }
    token = grn_token_cursor_get_token(ctx, cursor);
    GRN_RECORD_PUT(ctx, token_ids, token_id);
    location.offset = grn_token_get_source_offset(ctx, token);
    location.length = grn_token_get_source_length(ctx, token);
    location.is_overlapping = grn_token_is_overlap(ctx, token);
    {
      const char *data;
      size_t data_length;

      data = grn_token_get_data_raw(ctx, token, &data_length);
      location.first_character_length =
        grn_charlen_(ctx, data, data + data_length, encoding);
    }
    GRN_TEXT_PUT(ctx, token_locations, &location, sizeof(location));
  }
  grn_token_cursor_close(ctx, cursor);

  GRN_BULK_REWIND(candidates);
  {
    grn_pat *chunks = (grn_pat *)(highlighter->lexicon.token_id_chunks);
    size_t i;
    size_t n_token_ids = GRN_BULK_VSIZE(token_ids) / sizeof(grn_id);
    grn_id *raw_token_ids = (grn_id *)GRN_BULK_HEAD(token_ids);
    grn_highlighter_location *raw_token_locations =
      (grn_highlighter_location *)GRN_BULK_HEAD(token_locations);

    for (i = 0; i < n_token_ids; i++) {
      grn_id chunk_id;

      chunk_id = grn_pat_lcp_search(ctx,
                                    chunks,
                                    raw_token_ids + i,
                                    (n_token_ids - i) * sizeof(grn_id));
      if (chunk_id == GRN_ID_NIL) {
        continue;
      }

      {
        uint32_t key_size;
        size_t j;
        size_t n_ids;
        grn_highlighter_location candidate;
        grn_highlighter_location *first = raw_token_locations + i;

        _grn_pat_key(ctx, chunks, chunk_id, &key_size);
        n_ids = key_size / sizeof(grn_id);
        candidate.offset = first->offset;
        if (first->is_overlapping && n_ids > 1) {
          candidate.length = first->first_character_length;
        } else {
          candidate.length = first->length;
        }
        for (j = 1; j < n_ids; j++) {
          grn_highlighter_location *current = raw_token_locations + i + j;
          grn_highlighter_location *previous = current - 1;
          uint32_t current_length;
          uint32_t previous_length;
          if (current->is_overlapping && j + 1 < n_ids) {
            current_length = current->first_character_length;
          } else {
            current_length = current->length;
          }
          if (previous->is_overlapping && j + 1 < n_ids) {
            previous_length = previous->first_character_length;
          } else {
            previous_length = previous->length;
          }
          if (current->offset == previous->offset) {
            if (current_length > previous_length) {
              candidate.length += current_length - previous_length;
            }
          } else {
            candidate.length += current_length;
          }
        }
        GRN_TEXT_PUT(ctx, candidates, &candidate, sizeof(candidate));
        i += n_ids - 1;
      }
    }
  }

  {
    size_t i;
    size_t n_candidates =
      GRN_BULK_VSIZE(candidates) / sizeof(grn_highlighter_location);
    grn_highlighter_location *raw_candidates =
      (grn_highlighter_location *)GRN_BULK_HEAD(candidates);

    if (n_candidates == 0) {
      grn_text_escape_xml(ctx, output, text, text_length);
    } else {
      grn_highlighter_location *previous = NULL;
      uint64_t offset = 0;

      qsort(raw_candidates,
            n_candidates,
            sizeof(grn_highlighter_location),
            grn_highlighter_location_compare);
      previous = raw_candidates;
      for (i = 1; i < n_candidates; i++) {
        grn_highlighter_location *current = raw_candidates + i;
        if (previous->offset + previous->length >= current->offset) {
          if (previous->offset + previous->length >
              current->offset + current->length) {
            current->length = previous->length;
          } else {
            current->length += current->offset - previous->offset;
          }
          current->offset = previous->offset;
          previous = current;
          continue;
        }
        if (current->offset > previous->offset) {
          if (previous->offset > offset) {
            grn_text_escape_xml(ctx,
                                output,
                                text + offset,
                                previous->offset - offset);
          }
          GRN_TEXT_PUT(ctx,
                       output,
                       highlighter->tag.open,
                       highlighter->tag.open_length);
          grn_text_escape_xml(ctx,
                              output,
                              text + previous->offset,
                              previous->length);
          offset = previous->offset + previous->length;
          GRN_TEXT_PUT(ctx,
                       output,
                       highlighter->tag.close,
                       highlighter->tag.close_length);
        }
        previous = current;
      }
      if (previous->offset > offset) {
        grn_text_escape_xml(ctx,
                            output,
                            text + offset,
                            previous->offset - offset);
      }
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.open,
                   highlighter->tag.open_length);
      grn_text_escape_xml(ctx,
                          output,
                          text + previous->offset,
                          previous->length);
      offset = previous->offset + previous->length;
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.close,
                   highlighter->tag.close_length);
      if (offset < text_length) {
        grn_text_escape_xml(ctx,
                            output,
                            text + offset,
                            text_length - offset);
      }
    }
  }
}

static void
grn_highlighter_highlight_patricia_trie(grn_ctx *ctx,
                                        grn_highlighter *highlighter,
                                        const char *text,
                                        size_t text_length,
                                        grn_obj *output)
{
  const char *current = text;
  size_t current_length = text_length;

  while (current_length > 0) {
#define MAX_N_HITS 16
    grn_pat_scan_hit hits[MAX_N_HITS];
    const char *rest;
    int i, n_hits;
    size_t previous_length = 0;
    size_t chunk_length;

    n_hits = grn_pat_scan(ctx,
                          (grn_pat *)(highlighter->pat.keywords),
                          current, current_length,
                          hits, MAX_N_HITS,
                          &rest);
    for (i = 0; i < n_hits; i++) {
      if ((hits[i].offset - previous_length) > 0) {
        grn_text_escape_xml(ctx,
                            output,
                            current + previous_length,
                            hits[i].offset - previous_length);
      }
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.open,
                   highlighter->tag.open_length);
      grn_text_escape_xml(ctx,
                          output,
                          current + hits[i].offset,
                          hits[i].length);
      GRN_TEXT_PUT(ctx,
                   output,
                   highlighter->tag.close,
                   highlighter->tag.close_length);
      previous_length = hits[i].offset + hits[i].length;
    }

    chunk_length = rest - current;
    if ((chunk_length - previous_length) > 0) {
      grn_text_escape_xml(ctx,
                          output,
                          current + previous_length,
                          current_length - previous_length);
    }
    current_length -= chunk_length;
    current = rest;
#undef MAX_N_HITS
  }
}

grn_rc
grn_highlighter_highlight(grn_ctx *ctx,
                          grn_highlighter *highlighter,
                          const char *text,
                          int64_t text_length,
                          grn_obj *output)
{
  GRN_API_ENTER;

  if (text_length < 0) {
    text_length = strlen(text);
  }

  if (grn_vector_size(ctx, &(highlighter->raw_keywords)) == 0) {
    if (highlighter->is_html_mode) {
      grn_text_escape_xml(ctx,
                          output,
                          text,
                          text_length);
    } else {
      GRN_TEXT_PUT(ctx, output, text, text_length);
    }
    goto exit;
  }

  if (highlighter->need_prepared) {
    grn_highlighter_prepare(ctx, highlighter);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  if (highlighter->lexicon.object) {
    grn_highlighter_highlight_lexicon(ctx,
                                      highlighter,
                                      text,
                                      text_length,
                                      output);
  } else {
    grn_highlighter_highlight_patricia_trie(ctx,
                                            highlighter,
                                            text,
                                            text_length,
                                            output);
  }

exit :
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_highlighter_set_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            grn_obj *lexicon)
{
  GRN_API_ENTER;

  if (highlighter->lexicon.object != lexicon) {
    highlighter->need_prepared = GRN_TRUE;
    highlighter->lexicon.object = lexicon;
  }

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_highlighter_get_lexicon(grn_ctx *ctx,
                            grn_highlighter *highlighter)
{
  GRN_API_ENTER;

  GRN_API_RETURN(highlighter->lexicon.object);
}

grn_rc
grn_highlighter_add_keyword(grn_ctx *ctx,
                            grn_highlighter *highlighter,
                            const char *keyword,
                            int64_t keyword_length)
{
  GRN_API_ENTER;

  if (keyword_length < 0) {
    keyword_length = strlen(keyword);
  }

  if (keyword_length == 0) {
    goto exit;
  }

  grn_vector_add_element(ctx,
                         &(highlighter->raw_keywords),
                         keyword,
                         keyword_length,
                         0,
                         GRN_DB_TEXT);
  highlighter->need_prepared = GRN_TRUE;

exit :
  GRN_API_RETURN(ctx->rc);
}
