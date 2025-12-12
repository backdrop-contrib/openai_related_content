<?php
// Intentionally left blank.
//
// We ship the default view the Backdrop way via config/install:
//   config/install/views.view.openai_related_content_related.json
// If we also provided hook_views_default_views(), sites could see duplicate
// views or conflicting definitions. To avoid that, we keep this file as an
// empty placeholder so that the includes/ directory remains available for
// other Views integrations while the default View comes from configuration.
