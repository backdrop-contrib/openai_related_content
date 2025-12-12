## Quick orientation

This repository is a small Backdrop CMS module named `openai_related_content` that displays related nodes using OpenAI embeddings and a vector search service.

Key files:
- `openai_related_content.module` — main implementation (block registration, embedding calls, vector search, caching).
- `openai_related_content.info` — module metadata (Backdrop target, dependencies: `openai`, `search_api`, `search_api_ai`, `openai_embeddings`).
- `openai_related_content.css` — responsive grid styling used by the block.
- `README.md` and `LICENSE.txt` — minimal docs and license.

## High-level architecture & intent

- Purpose: compute an embedding for the current node and show a small set (3 by default) of related nodes found via a vector search.
- Data flow:
  1. On a canonical `node/%` page the module extracts text from the node (`openai_related_content_get_text`).
 2. It calls `openai_related_content_get_embedding()` which uses the OpenAI API wrapper configured in site config.
 3. The resulting embedding is passed to `openai_related_content_vector_search()` which asks the vector client for nearest neighbors from collection `amafoundation`.
 4. Returned nids are `node_load`-ed and rendered as teasers inside a responsive grid; results are cached under key `openai_related_content:<nid>` for 1 week.

Design notes you should preserve when editing:
- The block only renders on canonical node pages via `arg()` and `menu_get_object()` checks in `hook_block_view()` — modify carefully if altering where the block appears.
- Language/field access: the module reads `body['und'][0]['safe_value']` (language code `'und'`) — if you broaden field extraction, match Backdrop field structures.
- Caching is explicit: cache key `openai_related_content:<nid>` and 604800s TTL. Preserve or intentionally change the key/TTL and note vector DB sync implications.

## Project-specific patterns & helpers

- API key retrieval: the code reads `config('openai.settings')` and calls `key_get_key_value($openai_config->get('api_key'))`. Do not hardcode API keys — use this pattern for secrets.
- Embedding model default: pulled from `openai_embeddings.settings` with fallback to `text-embedding-3-small`. If you change model handling, update the settings UI or docs.
- Vector client interface: `openai_embeddings_get_vector_client()` is expected elsewhere; the client exposes `search($collection, $embedding, $top_k, $outputFields)` and returns items with `nid` and `title`.
  - The code requests `top_k = 25` and then filters and limits to `$limit` (3). Maintain this two-step approach when tuning recall vs precision.
- External collection: the code hardcodes collection name `amafoundation`. If you rename the collection, update code and the corresponding vector DB.

## Concrete examples (copy/paste safe guidance)
- CSS attachment pattern: attach assets in the render array using the `attached` key and a `css` entry that points to the module stylesheet at `backdrop_get_path('module', 'openai_related_content') . '/openai_related_content.css'`.
- Node rendering: the module uses `node_view($related_node, 'teaser')` then `render($view)` and strips certain links from `$view['links']`.

## Debugging & developer workflows (what actually works here)

- There is no automated test or build system in the repo. To test changes:
  - Install/enable the module inside a Backdrop site (admin/modules) or enable via Drush if you use it (`drush en openai_related_content`).
  - Ensure dependencies are installed and configured: the `openai` and `openai_embeddings` modules must be present and configured with an API key and vector client.
  - Verify the vector collection (`amafoundation`) exists in your vector DB and contains node `nid`/`title` fields returned by the client.
- For quick troubleshooting, inspect cache entries (Backdrops `cache_get`/`cache_set` usage) and check watchdog/dblog or PHP error logs for runtime exceptions from `OpenAIApi` or the vector client.

## Safe change checklist for PR authors

If you change behavior, confirm these before merging:
- If you change the collection name or output fields, update the vector DB schema and any ingestion jobs.
- If you alter the embedding model or API key handling, test with a real API key via site config (`openai.settings`).
- Preserve or intentionally update the cache key naming and TTL and document the reason in the PR description.

## Where to look next in the codebase

- `openai_related_content.module` — implementation details described above (primary source of truth).
- `openai_related_content.info` — dependencies and Backdrop target; heed this when adding libraries or changing the target platform.

## Views capability

- **New:** The module includes Views integration to provide a configurable listing of related content. Look for view default definitions and helpers under the `includes` directory and the `config/install` folder where a default view export (`views.view.openai_related_content_related.json`) is provided.
- **Files:** The Views integration is implemented via `includes/openai_related_content.views.inc` and `includes/openai_related_content.views_default.inc` and the exported view lives at `config/install/views.view.openai_related_content_related.json`.
- **When editing:** If you change view handlers, argument plugins, or the view export, update the exported JSON and ensure any Views plugin classes under `includes/plugins/` remain compatible.

If anything here is unclear or you want the instructions to include additional examples (e.g., sample Drush commands, a small test harness, or a note on the vector DB ingestion process), tell me what to expand and I will iterate.
