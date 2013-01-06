<?php
defined('_JEXEC') or die;

/**
 * Floating Box
 * @description:
 */

JHTML::_('behavior.mootools');

$type = (int) $params->get('type', 0);
$style1 = $params->get('style1', '');
$style2 = $params->get('style2', '');
$html = $params->get('html', '');
$relativeedge = (int) $params->get('relativeedge', 0); // 0 - top
$id = 'floating_box_'.$module->id;

echo '<div id="'.$id.'">'.$html.'</div>';
?>

<script type="text/javascript">
window.addEvent('scroll', function() {
    var elem = $(<?php ec),
        windowH = window.getSize().y,
        offsetY = window.getScroll().y,
        documentH = window.getScrollSize().y;

    elem.setStyle('height', elem.getSize().y);
    var parentH = elem.getParent().getSize().y,
        parentY = elem.getParent().getOffsets().y,
        floatElem = elem.getFirst();

    if (offsetY + windowH >= parentY + parentH -
elem.getStyle('marginBottom').toInt()) {
        floatElem.setStyles({'position': '', 'bottom': '', 'width': ''});
    } else {
        if (!floatElem.style.width) {
            floatElem.setStyle('width', floatElem.getSize().x+'px')
        }
        floatElem.setStyles({'position': 'fixed','bottom': 0});
        }
    });
});
</script>