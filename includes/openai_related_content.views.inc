<?php
/**
 * @file
 * Views integration for OpenAI Related Content.
 */

/**
 * Implements hook_views_data_alter().
 */
function openai_related_content_views_data_alter(&$data) {
  // Override the default node.nid argument handler with our custom one
  // This makes our handler available as the default for Content: Nid arguments
  $data['node']['nid']['argument']['handler'] = 'openai_related_content_argument';
}
