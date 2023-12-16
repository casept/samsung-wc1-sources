/**
 * @internal
 * @defgroup Bubble Bubble
 * @ingroup Elementary
 *
 * @image html bubble_inheritance_tree.png
 * @image latex bubble_inheritance_tree.eps
 *
 * @image html img/widget/bubble/preview-00.png
 * @image latex img/widget/bubble/preview-00.eps
 * @image html img/widget/bubble/preview-01.png
 * @image latex img/widget/bubble/preview-01.eps
 * @image html img/widget/bubble/preview-02.png
 * @image latex img/widget/bubble/preview-02.eps
 *
 * @brief The Bubble is a widget to show text similar to how speech is
 *        represented in comics.
 *
 * The bubble widget contains 5 important visual elements:
 * @li The frame is a rectangle with rounded edjes and an "arrow".
 * @li The @a icon is an image to which the frame's arrow points to.
 * @li The @a label is a text which appears to the right of the icon if the
 * corner is "top_left" or "bottom_left" and is right aligned to the frame
 * otherwise.
 * @li The @a info is a text which appears to the right of the label. Info's
 * font is of a lighter color than label.
 * @li The @a content is an evas object that is shown inside the frame.
 *
 * The position of the arrow, icon, label and info depends on which corner is
 * selected. The four available corners are:
 * @li "top_left" - Default
 * @li "top_right"
 * @li "bottom_left"
 * @li "bottom_right"
 *
 * This widget inherits from the @ref Layout one, so that all the
 * functions acting on it also work for bubble objects.
 *
 * This widget emits the following signals, besides the ones sent from
 * @ref Layout:
 * @li @c "clicked" - This is called when a user has clicked the bubble.
 *
 * Default content parts of the bubble that you can use for are:
 * @li "default" - A content of the bubble
 * @li "icon" - An icon of the bubble
 *
 * Default text parts of the button widget that you can use for are:
 * @li "default" - Label of the bubble
 * @li "info" - info of the bubble
 *
 * Supported elm_object common APIs.
 * @li @ref elm_object_part_text_set
 * @li @ref elm_object_part_text_get
 * @li @ref elm_object_part_content_set
 * @li @ref elm_object_part_content_get
 * @li @ref elm_object_part_content_unset
 *
 * For an example of using a bubble see @ref bubble_01_example_page "this".
 *
 * @{
 */

/**
 * @brief Defines the corner values for a bubble.
 *
 * @remarks The corner is used to determine where the arrow of the
 *          bubble points to.
 */
typedef enum
{
  ELM_BUBBLE_POS_INVALID = -1,
  ELM_BUBBLE_POS_TOP_LEFT,
  ELM_BUBBLE_POS_TOP_RIGHT,
  ELM_BUBBLE_POS_BOTTOM_LEFT,
  ELM_BUBBLE_POS_BOTTOM_RIGHT,
} Elm_Bubble_Pos;

/**
 * @brief Adds a new bubble to the parent
 *
 * @details This function adds a text bubble to the given parent evas object
 *
 * @param[in] parent The parent object
 * @return The new object, otherwise @c NULL if it cannot be created
 */
EAPI Evas_Object                 *elm_bubble_add(Evas_Object *parent);

/**
 * @brief Sets the corner of the bubble
 *
 * @details This function sets the corner of the bubble. The corner is used to
 *          determine where the arrow in the frame points to and where label, icon, and
 *          info are shown.
 *
 * @param[in] obj The bubble object
 * @param[in] pos The given corner for the bubble
 */
EAPI void  elm_bubble_pos_set(Evas_Object *obj, Elm_Bubble_Pos pos);

/**
 * @brief Gets the corner of the bubble.
 *
 * @details This function gets the selected corner of the bubble.
 *
 * @param[in] obj The bubble object
 * @return The given corner for the bubble
 */
EAPI Elm_Bubble_Pos elm_bubble_pos_get(const Evas_Object *obj);

/**
 * @}
 */
