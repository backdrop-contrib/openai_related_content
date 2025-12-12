<?php

/**
 * @file
 * Views integration for OpenAI Related Content.
 */

/**
 * Implements hook_views_data().
 */
function openai_related_content_views_data() {
  $data = [];

  // Create a new pseudo-field that can be used as a contextual filter
  $data['openai_related_content']['table']['group'] = t('OpenAI Related Content');

  $data['openai_related_content']['table']['join'] = [
    '#global' => [],
  ];

  $data['openai_related_content']['nid'] = [
    'title' => t('OpenAI Related Content'),
    'help' => t('Finds nodes related to the given node ID using AI vector similarity.'),
    'argument' => [
      'handler' => 'openai_related_content_handler_argument',
    ],
  ];

  return $data;
}
