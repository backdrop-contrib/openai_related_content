# OpenAI Related Content - Troubleshooting Guide

## Overview

This guide documents the issues encountered during development and the solutions implemented to make the OpenAI Related Content module work properly with vector databases (Milvus and Pinecone).

## Initial Problem

The related content module was not returning any related nodes on the test page, despite embeddings being generated successfully. The `$related` variable came back empty from the vector search.

## Root Causes Identified

### 1. Limited Text Extraction

**Problem:** The original `openai_related_content_get_text()` function only extracted text from `$node->body['und'][0]['safe_value']`, but modern Backdrop sites use Paragraphs, Layout Builder, and other field types for content.

**Impact:** Nodes with content in Paragraphs or other fields had very little or no text extracted, resulting in poor embeddings and no related content matches.

**Solution:** Implemented comprehensive text extraction that:
- Renders the full node in 'full' view mode using `node_view()`
- Extracts all visible text content from Paragraphs, Layout Builder, and field-based structures
- Falls back to field-by-field extraction if rendering fails
- Handles taxonomy terms and other semantic content
- Normalizes whitespace and limits text length to 8000 characters

### 2. Node ID Extraction Logic Issues

**Problem:** The vector databases already stored the node ID correctly, but the extraction logic in the module wasn't properly accessing it from the search results.

**Milvus Metadata Example:**
```json
{
  "backdrop_entity_id": "entity:node/61:en",
  "backdrop_long_id": "entity:node/61:en:search_api_viewed:0",
  "nid": 61,
  "title": "Matthew J. Harmon",
  "content": "Matthew J. Harmon EditDelete..."
}
```

**Pinecone Metadata Example:**
```
ID: entity:node/100:en:search_api_viewed:0
metadata:
  content: "Funds EditRemoveAdd new Accordions..."
  search_api_url: "funds"
  title: "Funds"
  url: "https://amafoundation-backdrop.ddev.site/funds"
```

**Key Insight:**
- Milvus stored `"nid": 61` directly in the metadata
- Pinecone stored the nid in the vector ID format: `entity:node/100:en:...`

The original extraction logic wasn't handling these different structures properly.

**Solution:** Enhanced node ID extraction to check multiple sources:
- `metadata['nid']` - Direct nid field (Milvus)
- `metadata['entity_id']` - Entity ID field
- Vector ID parsing: `entity:node/123:en:field:chunk` (Pinecone)
- Multiple regex patterns for robustness

### 3. Database Table Missing

**Problem:** The exclusion table `openai_related_content_exclude` didn't exist, causing SQL errors when checking node exclusions.

**Impact:** SQL errors in watchdog logs: `SQLSTATE[42S22]: Column not found: 1054 Unknown column '1' in 'where clause'`

**Solution:**
- Added `db_table_exists()` checks before querying the exclusion table
- Wrapped database operations in try/catch blocks to prevent crashes
- Ensured proper table creation during module updates via `hook_schema()`

### 4. Silent Vector Search Failures

**Problem:** Search failures weren't logged, making debugging impossible.

**Impact:** Developers couldn't diagnose why searches were failing.

**Solution:** Added comprehensive debug logging throughout the search pipeline:
- Text extraction length and content preview
- Embedding generation success/failure with dimension counts
- Vector search parameters (namespace, collection, top_k, embedding dimensions)
- Raw vector database responses with sample matches
- Filtering statistics (why matches were excluded: no_nid, current_node, duplicates, excluded, unpublished)

## What Was NOT the Problem

### Vector Database Metadata Was Already Correct

The metadata stored in both Milvus and Pinecone already contained all the necessary information to identify nodes:

- **Milvus:** Had `"nid": 61` directly in the stored metadata
- **Pinecone:** Had node ID embedded in the vector ID format

The issue was purely in the **extraction logic** - not in how the data was stored. While we added `nid`, `entity_id`, and `entity_type` to the metadata normalization function for future consistency, this wasn't strictly required since the data was already there.

## Code Changes Made

### Text Extraction Enhancement (`openai_related_content.module`)

**Before:**
```php
function openai_related_content_get_text($node) {
  $fields = [$node->title];
  if (!empty($node->body['und'][0]['safe_value'])) {
    $fields[] = strip_tags($node->body['und'][0]['safe_value']);
  }
  return implode(' ', $fields);
}
```

**After:**
```php
function openai_related_content_get_text($node) {
  // Render full node content instead of just body field
  $build = node_view($node, 'full');
  $rendered = backdrop_render($build);
  $plain_text = html_entity_decode(strip_tags($rendered), ENT_QUOTES, 'UTF-8');
  // Normalize whitespace and truncate...
}
```

### Node ID Extraction (`openai_related_content.module`)

**Before:** Only checked `$metadata['nid']`

**After:** Multiple fallback patterns:
```php
// Try multiple ways to extract the node ID from metadata
if (!empty($metadata['nid'])) {
  $nid = $metadata['nid'];
}
elseif (!empty($metadata['entity_id']) && $metadata['entity_type'] === 'node') {
  $nid = $metadata['entity_id'];
}
elseif (!empty($match['id'])) {
  // Extract from Pinecone vector ID format: entity:node/123:en:field:chunk
  if (preg_match('/entity[:\-_]?node[:\-_\/]?(\d+)/i', $match['id'], $m)) {
    $nid = $m[1];
  }
}
```

### Error Handling (`openai_related_content.module`)

```php
try {
  if (db_table_exists('openai_related_content_exclude')) {
    $result = db_query("SELECT 1 FROM {openai_related_content_exclude} WHERE nid = :nid", [':nid' => $nid])->fetchField();
  }
}
catch (Exception $e) {
  watchdog('openai_related_content', 'Error checking exclusion: @error', ['@error' => $e->getMessage()], WATCHDOG_WARNING);
}
```

## Testing and Validation

### Test Page

The test page (`/admin/config/content/openai-related-content/test`) now shows:
1. **Step 1: Text Extraction** - Character count and content preview
2. **Step 2: Embedding Generation** - Success/failure with dimension count
3. **Step 3: Vector Search** - Results and filtering statistics

### Debug Logging

Watchdog logs (`/admin/reports/dblog?type=openai_related_content`) now include:
- Search parameters and raw responses
- Filtering statistics showing why matches were excluded
- Sample match data for debugging

### UI Improvements

- Added "Test Related Content Functionality" button on settings page
- Removed redundant debug tools section from test page

## Lessons Learned

1. **Check extraction logic first** - The data may be stored correctly but not retrieved properly
2. **Handle different backend response structures** - Milvus and Pinecone return data differently
3. **Log everything during debugging** - Silent failures make diagnosis impossible
4. **Don't assume content structure** - Modern CMS sites use complex field arrangements (Paragraphs, etc.)
5. **Handle missing database tables gracefully** - Tables may not exist during initial setup

## Summary

The primary fixes were:

| Issue | Root Cause | Solution |
|-------|------------|----------|
| No related content | Text extraction only used body field | Render full node and extract all text |
| Empty search results | Extraction logic didn't handle Milvus/Pinecone response formats | Added multiple extraction patterns |
| SQL errors | Missing exclusion table | Added table existence checks and try/catch |
| Debugging impossible | No logging | Added comprehensive debug logging |

The vector databases were storing metadata correctly all along - the issue was in how the module was extracting and processing that data.
