<?php

/**
 * @file
 * Views argument default plugin: returns NIDs related to the current node.
 *
 * NOTE: This plugin is primarily for backward compatibility. The recommended
 * approach is to use the custom argument handler (openai_related_content_argument)
 * which properly handles both preview and live contexts.
 */

/**
 * Argument default plugin that supplies related node IDs.
 *
 * This plugin can be used with standard Views argument handlers to provide
 * a comma-separated list of related node IDs as the default argument value.
 */
class openai_related_content_plugin_argument_default_related extends views_plugin_argument_default {

  /**
   * Provide a form to select the search index for this view.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    // Get all available Search API indexes
    $indexes = search_api_index_load_multiple(FALSE);
    $index_options = [];
    foreach ($indexes as $index_id => $index) {
      $index_options[$index_id] = $index->name;
    }

    $form['index_id'] = [
      '#type' => 'select',
      '#title' => t('Search API Index'),
      '#description' => t('Select the Search API index to use for finding related content. This overrides the global module setting for this view.'),
      '#options' => $index_options,
      '#default_value' => $this->options['index_id'] ?? '',
      '#required' => TRUE,
    ];
  }

  /**
   * Return a comma-separated list of NIDs related to the current node.
   */
  public function get_argument() {
    // Only on node/% pages.
    if (arg(0) !== 'node' || !is_numeric(arg(1)) || !empty(arg(2))) {
      return '';
    }

    $node = menu_get_object('node');
    if (!$node || empty($node->nid)) {
      return '';
    }

    // Use the module config, but always get MORE items than needed
    // The view's pager will control how many actually display
    $config = config('openai_related_content.settings');
    $limit = 10; // Always get 10 related items, let view pager limit display

    // Use the index from options, or fall back to global config
    $index_id = $this->options['index_id'] ?? NULL;
    if (!$index_id) {
      $config = config('openai_related_content.settings');
      $index_id = $config->get('index_id');
    }

    $matches = openai_related_content_get_related_matches($node, $limit, $index_id);
    if (empty($matches)) {
      return '';
    }

    $nids = [];
    foreach ($matches as $match) {
      if (!empty($match['nid'])) {
        $nids[] = (int) $match['nid'];
      }
    }

    return implode(',', $nids);
  }
}
