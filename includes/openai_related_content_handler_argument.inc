<?php

/**
 * @file
 * Argument handler for OpenAI Related Content.
 */

/**
 * Argument handler to find nodes related to a given node.
 */
class openai_related_content_handler_argument extends views_handler_argument_numeric {

  /**
   * Override query to filter by related node IDs.
   */
  function query($group_by = FALSE) {
    $this->ensure_my_table();

    // Get the source node ID from the argument
    $source_nid = $this->argument;
    
    if (!is_numeric($source_nid) || $source_nid <= 0) {
      // Invalid argument - show no results
      $this->query->add_where(0, '1', '0', '=');
      return;
    }

    // Load the source node
    $node = node_load($source_nid);
    if (!$node) {
      // Node not found - show no results
      $this->query->add_where(0, '1', '0', '=');
      return;
    }

    // Get the limit from the view's pager
    $limit = 3; // Default
    if (!empty($this->view->pager['items_per_page'])) {
      $limit = (int) $this->view->pager['items_per_page'];
    }
    elseif (!empty($this->view->display_handler)) {
      $pager = $this->view->display_handler->get_option('pager');
      if (!empty($pager['options']['items_per_page'])) {
        $limit = (int) $pager['options']['items_per_page'];
      }
    }

    // Get related node IDs
    $config = config('openai_related_content.settings');
    $index_id = $config->get('index_id');
    $related = openai_related_content_get_related_matches($node, $limit, $index_id);

    if (empty($related)) {
      // No related content - show no results
      $this->query->add_where(0, '1', '0', '=');
      return;
    }

    // Extract the NIDs
    $nids = [];
    foreach ($related as $match) {
      if (!empty($match['nid'])) {
        $nids[] = (int) $match['nid'];
      }
    }

    if (empty($nids)) {
      // No valid NIDs - show no results
      $this->query->add_where(0, '1', '0', '=');
      return;
    }

    // Add WHERE clause to filter by these NIDs
    $this->query->add_where(0, 'node.nid', $nids, 'IN');
  }

  /**
   * Override title to show the source node's title.
   */
  function title() {
    if (is_numeric($this->argument) && $this->argument > 0) {
      $node = node_load($this->argument);
      if ($node) {
        return check_plain($node->title);
      }
    }
    return t('Related Content');
  }
}
