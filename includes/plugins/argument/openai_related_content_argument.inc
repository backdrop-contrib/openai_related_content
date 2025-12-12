<?php

/**
 * @file
 * Contains the OpenAI Related Content argument handler.
 */

/**
 * Argument handler for OpenAI Related Content.
 *
 * This handler takes a node ID as input and converts it to a list of related
 * node IDs based on vector similarity, then lets the parent handler process them.
 */
class openai_related_content_argument extends views_handler_argument_numeric {

  /**
   * Override query to intercept the node ID and replace it with related NIDs.
   */
  function query($group_by = FALSE) {
    // Get the original argument (the source node ID)
    $source_nid = $this->argument;

    if (!is_numeric($source_nid)) {
      // Invalid argument, let parent handle it (will show no results)
      parent::query($group_by);
      return;
    }

    // Load the source node
    $node = node_load($source_nid);
    if (!$node) {
      // Node not found, let parent handle it (will show no results)
      parent::query($group_by);
      return;
    }

    // Get the view's items_per_page setting to know how many to fetch
    $limit = NULL;
    $display_all = FALSE;

    if (!empty($this->view->display_handler)) {
      $pager = $this->view->display_handler->get_option('pager');

      // Check pager type and get limit accordingly
      if (isset($pager['type'])) {
        if ($pager['type'] === 'none') {
          // Display all items - no pager
          $display_all = TRUE;
        }
        elseif ($pager['type'] === 'some' && isset($pager['options']['items_per_page'])) {
          // Display a specified number of items
          $limit = (int) $pager['options']['items_per_page'];
          if ($limit === 0) {
            $display_all = TRUE;
          }
        }
        elseif ($pager['type'] === 'full' && isset($pager['options']['items_per_page'])) {
          // Full pager
          $limit = (int) $pager['options']['items_per_page'];
          if ($limit === 0) {
            $display_all = TRUE;
          }
        }
        elseif ($pager['type'] === 'mini' && isset($pager['options']['items_per_page'])) {
          // Mini pager
          $limit = (int) $pager['options']['items_per_page'];
          if ($limit === 0) {
            $display_all = TRUE;
          }
        }
      }
    }

    // Set limit based on what we found
    if ($display_all) {
      // For "display all", fetch many related items (up to 100)
      $limit = 100;
    }
    elseif ($limit === NULL) {
      // Fallback to config if view doesn't specify
      $config = config('openai_related_content.settings');
      $limit = (int) ($config->get('limit') ?: 3);
    }

    // Get related node IDs using the module's function
    $config = config('openai_related_content.settings');
    $index_id = $config->get('index_id');
    $related = openai_related_content_get_related_matches($node, $limit, $index_id);

    if (empty($related)) {
      // No related content, let parent handle it (will show no results)
      parent::query($group_by);
      return;
    }

    // Extract just the NIDs in order
    $nids = [];
    foreach ($related as $match) {
      if (!empty($match['nid'])) {
        $nids[] = (int) $match['nid'];
      }
    }

    if (empty($nids)) {
      parent::query($group_by);
      return;
    }

    // REPLACE the argument with the comma-separated list of related NIDs
    // This makes the parent query() method process them as if the user entered them
    $this->argument = implode(',', $nids);

    // Store the original order for potential use by sort handlers
    $this->view->openai_related_nids_order = $nids;

    // Now call the parent query() which will process our replaced argument
    // with break_phrase to create the IN() query
    parent::query($group_by);
  }

  /**
   * Override title to show the source node's title.
   */
  function title() {
    // The argument might have been replaced with multiple NIDs,
    // but we want to show the title of the ORIGINAL source node
    if (!empty($this->argument) && is_numeric($this->argument)) {
      $node = node_load($this->argument);
      if ($node) {
        return check_plain($node->title);
      }
    }
    // If argument was already replaced, try to get the first NID
    if (!empty($this->value) && is_array($this->value) && isset($this->value[0])) {
      $node = node_load($this->value[0]);
      if ($node) {
        return check_plain($node->title);
      }
    }
    return parent::title();
  }

}

