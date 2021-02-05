TARGET = corelib
TEMPLATE = lib

CONFIG += c++1z
QT += widgets widgets-private sql xml

DEFINES += CORE_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../_build

INCLUDEPATH += ..

#
# Подключаем библиотеку fileformats
#
LIBS += -L$$DESTDIR/libs/ -lfileformats
win32-msvc*:QMAKE_LFLAGS += /WHOLEARCHIVE:$$DESTDIR/libs/fileformats.lib

INCLUDEPATH += $$PWD/../3rd_party/fileformats
DEPENDPATH += $$PWD/../3rd_party/fileformats
PRE_TARGETDEPS += $$PWD/../3rd_party/fileformats

mac:LIBS += -lz
#

#
# Подключаем библиотеку HUNSPELL
#
LIBS += -L$$DESTDIR/libs/ -lhunspell
win32-msvc*:QMAKE_LFLAGS += /WHOLEARCHIVE:$$DESTDIR/libs/hunspell.lib

INCLUDEPATH += $$PWD/../3rd_party/hunspell/src
DEPENDPATH += $$PWD/../3rd_party/hunspell
PRE_TARGETDEPS += $$PWD/../3rd_party/hunspell
#

#
# Подключаем библиотеку qgumboparser
#
LIBS += -L$$DESTDIR/libs/ -lqgumboparser
win32-msvc*:QMAKE_LFLAGS += /WHOLEARCHIVE:$$DESTDIR/libs/qgumboparser.lib

INCLUDEPATH += $$PWD/../3rd_party/qgumboparser
DEPENDPATH += $$PWD/../3rd_party/qgumboparser
PRE_TARGETDEPS += $$PWD/../3rd_party/qgumboparser
#

SOURCES += \
    business_layer/chronometry/chronometer.cpp \
    business_layer/import/abstract_importer.cpp \
    business_layer/import/celtx_importer.cpp \
    business_layer/import/document_importer.cpp \
    business_layer/import/fdx_importer.cpp \
    business_layer/import/fountain_importer.cpp \
    business_layer/import/kit_scenarist_importer.cpp \
    business_layer/import/trelby_importer.cpp \
    business_layer/model/abstract_model.cpp \
    business_layer/model/abstract_model_item.cpp \
    business_layer/model/characters/character_model.cpp \
    business_layer/model/characters/characters_model.cpp \
    business_layer/model/locations/location_model.cpp \
    business_layer/model/locations/locations_model.cpp \
    business_layer/model/project/project_information_model.cpp \
    business_layer/model/recycle_bin/recycle_bin_model.cpp \
    business_layer/model/screenplay/screenplay_dictionaries_model.cpp \
    business_layer/model/screenplay/screenplay_information_model.cpp \
    business_layer/model/screenplay/screenplay_statistics_model.cpp \
    business_layer/model/screenplay/screenplay_synopsis_model.cpp \
    business_layer/model/screenplay/screenplay_treatment_model.cpp \
    business_layer/model/screenplay/text/screenplay_text_block_parser.cpp \
    business_layer/model/screenplay/text/screenplay_text_model.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_folder_item.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_item.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_scene_item.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_splitter_item.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_text_item.cpp \
    business_layer/model/screenplay/screenplay_title_page_model.cpp \
    business_layer/model/screenplay/text/screenplay_text_model_xml.cpp \
    business_layer/model/structure/structure_model.cpp \
    business_layer/model/structure/structure_model_item.cpp \
    business_layer/model/structure/structure_proxy_model.cpp \
    business_layer/model/text/text_model.cpp \
    business_layer/templates/screenplay_template.cpp \
    business_layer/templates/screenplay_template_facade.cpp \
    data_layer/database.cpp \
    data_layer/mapper/abstract_mapper.cpp \
    data_layer/mapper/document_change_mapper.cpp \
    data_layer/mapper/document_mapper.cpp \
    data_layer/mapper/mapper_facade.cpp \
    data_layer/mapper/settings_mapper.cpp \
    data_layer/storage/document_change_storage.cpp \
    data_layer/storage/document_data_storage.cpp \
    data_layer/storage/document_storage.cpp \
    data_layer/storage/settings_storage.cpp \
    data_layer/storage/storage_facade.cpp \
    domain/document_change_object.cpp \
    domain/document_object.cpp \
    domain/domain_object.cpp \
    domain/identifier.cpp \
    domain/objects_builder.cpp \
    ui/design_system/design_system.cpp \
    ui/widgets/app_bar/app_bar.cpp \
    ui/widgets/button/button.cpp \
    ui/widgets/card/card.cpp \
    ui/widgets/chat/chat_message.cpp \
    ui/widgets/chat/chat_messages_view.cpp \
    ui/widgets/chat/user.cpp \
    ui/widgets/check_box/check_box.cpp \
    ui/widgets/circular_progress_bar/circular_progress_bar.cpp \
    ui/widgets/color_picker/color_2d_slider.cpp \
    ui/widgets/color_picker/color_hue_slider.cpp \
    ui/widgets/color_picker/color_palette.cpp \
    ui/widgets/color_picker/color_picker.cpp \
    ui/widgets/combo_box/combo_box.cpp \
    ui/widgets/context_menu/context_menu.cpp \
    ui/widgets/dialog/abstract_dialog.cpp \
    ui/widgets/dialog/dialog.cpp \
    ui/widgets/dialog/dialog_content.cpp \
    ui/widgets/dialog/standard_dialog.cpp \
    ui/widgets/drawer/drawer.cpp \
    ui/widgets/floating_tool_bar/floating_tool_bar.cpp \
    ui/widgets/floating_tool_bar/floating_toolbar_animator.cpp \
    ui/widgets/image_cropper/image_cropper.cpp \
    ui/widgets/label/label.cpp \
    ui/widgets/label/link_label.cpp \
    ui/widgets/radio_button/radio_button.cpp \
    ui/widgets/radio_button/radio_button_group.cpp \
    ui/widgets/scroll_bar/scroll_bar.cpp \
    ui/widgets/shadow/shadow.cpp \
    ui/widgets/slider/slider.cpp \
    ui/widgets/splitter/splitter.cpp \
    ui/widgets/stack_widget/stack_widget.cpp \
    ui/widgets/stepper/stepper.cpp \
    ui/widgets/tab_bar/tab_bar.cpp \
    ui/widgets/task_bar/task_bar.cpp \
    ui/widgets/text_edit/base/base_text_edit.cpp \
    ui/widgets/text_edit/completer/completer.cpp \
    ui/widgets/text_edit/completer/completer_text_edit.cpp \
    ui/widgets/text_edit/page/page_metrics.cpp \
    ui/widgets/text_edit/page/page_text_edit.cpp \
    ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.cpp \
    ui/widgets/text_edit/spell_check/spell_check_highlighter.cpp \
    ui/widgets/text_edit/spell_check/spell_check_text_edit.cpp \
    ui/widgets/text_edit/spell_check/spell_checker.cpp \
    ui/widgets/text_edit/spell_check/syntax_highlighter.cpp \
    ui/widgets/text_field/text_field.cpp \
    ui/widgets/toggle_button/toggle_button.cpp \
    ui/widgets/tree/tree.cpp \
    ui/widgets/tree/tree_delegate.cpp \
    ui/widgets/tree/tree_view.cpp \
    ui/widgets/widget/widget.cpp \
    utils/3rd_party/WAF/Animation/Animation.cpp \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillAnimator.cpp \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillDecorator.cpp \
    utils/3rd_party/WAF/Animation/CircleTransparent/CircleTransparentAnimator.cpp \
    utils/3rd_party/WAF/Animation/CircleTransparent/CircleTransparentDecorator.cpp \
    utils/3rd_party/WAF/Animation/Expand/ExpandAnimator.cpp \
    utils/3rd_party/WAF/Animation/Expand/ExpandDecorator.cpp \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideAnimator.cpp \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideDecorator.cpp \
    utils/3rd_party/WAF/Animation/Slide/SlideAnimator.cpp \
    utils/3rd_party/WAF/Animation/Slide/SlideForegroundDecorator.cpp \
    utils/diff_match_patch/diff_match_patch.cpp \
    utils/diff_match_patch/diff_match_patch_controller.cpp \
    utils/helpers/color_helper.cpp \
    utils/helpers/dialog_helper.cpp \
    utils/helpers/extension_helper.cpp \
    utils/helpers/image_helper.cpp \
    utils/helpers/scroller_helper.cpp \
    utils/helpers/string_helper.cpp \
    utils/helpers/text_helper.cpp \
    utils/helpers/quotes_helper.cpp \
    utils/helpers/time_helper.cpp \
    utils/tools/backup_builder.cpp \
    utils/tools/debouncer.cpp \
    utils/tools/model_index_path.cpp \
    utils/tools/run_once.cpp \
    utils/validators/email_validator.cpp

HEADERS += \
    business_layer/chronometry/chronometer.h \
    business_layer/import/abstract_importer.h \
    business_layer/import/celtx_importer.h \
    business_layer/import/document_importer.h \
    business_layer/import/fdx_importer.h \
    business_layer/import/fountain_importer.h \
    business_layer/import/import_options.h \
    business_layer/import/kit_scenarist_importer.h \
    business_layer/import/trelby_importer.h \
    business_layer/model/abstract_image_wrapper.h \
    business_layer/model/abstract_model.h \
    business_layer/model/abstract_model_item.h \
    business_layer/model/characters/character_model.h \
    business_layer/model/characters/characters_model.h \
    business_layer/model/locations/location_model.h \
    business_layer/model/locations/locations_model.h \
    business_layer/model/project/project_information_model.h \
    business_layer/model/recycle_bin/recycle_bin_model.h \
    business_layer/model/screenplay/screenplay_dictionaries_model.h \
    business_layer/model/screenplay/screenplay_information_model.h \
    business_layer/model/screenplay/screenplay_statistics_model.h \
    business_layer/model/screenplay/screenplay_synopsis_model.h \
    business_layer/model/screenplay/screenplay_treatment_model.h \
    business_layer/model/screenplay/text/screenplay_text_block_parser.h \
    business_layer/model/screenplay/text/screenplay_text_model.h \
    business_layer/model/screenplay/text/screenplay_text_model_folder_item.h \
    business_layer/model/screenplay/text/screenplay_text_model_item.h \
    business_layer/model/screenplay/text/screenplay_text_model_scene_item.h \
    business_layer/model/screenplay/text/screenplay_text_model_splitter_item.h \
    business_layer/model/screenplay/text/screenplay_text_model_text_item.h \
    business_layer/model/screenplay/screenplay_title_page_model.h \
    business_layer/model/screenplay/text/screenplay_text_model_xml.h \
    business_layer/model/structure/structure_model.h \
    business_layer/model/structure/structure_model_item.h \
    business_layer/model/structure/structure_proxy_model.h \
    business_layer/model/text/text_model.h \
    business_layer/templates/screenplay_template.h \
    business_layer/templates/screenplay_template_facade.h \
    corelib_global.h \
    data_layer/database.h \
    data_layer/mapper/abstract_mapper.h \
    data_layer/mapper/document_change_mapper.h \
    data_layer/mapper/document_mapper.h \
    data_layer/mapper/mapper_facade.h \
    data_layer/mapper/settings_mapper.h \
    data_layer/storage/document_change_storage.h \
    data_layer/storage/document_data_storage.h \
    data_layer/storage/document_storage.h \
    data_layer/storage/settings_storage.h \
    data_layer/storage/storage_facade.h \
    domain/document_change_object.h \
    domain/document_object.h \
    domain/domain_object.h \
    domain/identifier.h \
    domain/objects_builder.h \
    ui/abstract_navigator.h \
    ui/design_system/design_system.h \
    ui/widgets/app_bar/app_bar.h \
    ui/widgets/button/button.h \
    ui/widgets/card/card.h \
    ui/widgets/chat/chat_message.h \
    ui/widgets/chat/chat_messages_view.h \
    ui/widgets/chat/user.h \
    ui/widgets/check_box/check_box.h \
    ui/widgets/circular_progress_bar/circular_progress_bar.h \
    ui/widgets/color_picker/color_2d_slider.h \
    ui/widgets/color_picker/color_hue_slider.h \
    ui/widgets/color_picker/color_palette.h \
    ui/widgets/color_picker/color_picker.h \
    ui/widgets/combo_box/combo_box.h \
    ui/widgets/context_menu/context_menu.h \
    ui/widgets/dialog/abstract_dialog.h \
    ui/widgets/dialog/dialog.h \
    ui/widgets/dialog/dialog_content.h \
    ui/widgets/dialog/standard_dialog.h \
    ui/widgets/drawer/drawer.h \
    ui/widgets/floating_tool_bar/floating_tool_bar.h \
    ui/widgets/floating_tool_bar/floating_toolbar_animator.h \
    ui/widgets/image_cropper/image_cropper.h \
    ui/widgets/label/label.h \
    ui/widgets/label/link_label.h \
    ui/widgets/radio_button/radio_button.h \
    ui/widgets/radio_button/radio_button_group.h \
    ui/widgets/scroll_bar/scroll_bar.h \
    ui/widgets/shadow/shadow.h \
    ui/widgets/slider/slider.h \
    ui/widgets/splitter/splitter.h \
    ui/widgets/stack_widget/stack_widget.h \
    ui/widgets/stepper/stepper.h \
    ui/widgets/tab_bar/tab_bar.h \
    ui/widgets/task_bar/task_bar.h \
    ui/widgets/text_edit/base/base_text_edit.h \
    ui/widgets/text_edit/completer/completer.h \
    ui/widgets/text_edit/completer/completer_text_edit.h \
    ui/widgets/text_edit/page/page_metrics.h \
    ui/widgets/text_edit/page/page_text_edit.h \
    ui/widgets/text_edit/page/page_text_edit_p.h \
    ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h \
    ui/widgets/text_edit/spell_check/spell_check_highlighter.h \
    ui/widgets/text_edit/spell_check/spell_check_text_edit.h \
    ui/widgets/text_edit/spell_check/spell_checker.h \
    ui/widgets/text_edit/spell_check/syntax_highlighter.h \
    ui/widgets/text_field/text_field.h \
    ui/widgets/toggle_button/toggle_button.h \
    ui/widgets/tree/tree.h \
    ui/widgets/tree/tree_delegate.h \
    ui/widgets/tree/tree_view.h \
    ui/widgets/widget/widget.h \
    utils/3rd_party/WAF/AbstractAnimator.h \
    utils/3rd_party/WAF/Animation/Animation.h \
    utils/3rd_party/WAF/Animation/AnimationPrivate.h \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillAnimator.h \
    utils/3rd_party/WAF/Animation/CircleFill/CircleFillDecorator.h \
    utils/3rd_party/WAF/Animation/CircleTransparent/CircleTransparentAnimator.h \
    utils/3rd_party/WAF/Animation/CircleTransparent/CircleTransparentDecorator.h \
    utils/3rd_party/WAF/Animation/Expand/ExpandAnimator.h \
    utils/3rd_party/WAF/Animation/Expand/ExpandDecorator.h \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideAnimator.h \
    utils/3rd_party/WAF/Animation/SideSlide/SideSlideDecorator.h \
    utils/3rd_party/WAF/Animation/Slide/SlideAnimator.h \
    utils/3rd_party/WAF/Animation/Slide/SlideForegroundDecorator.h \
    utils/3rd_party/WAF/WAF.h \
    utils/diff_match_patch/diff_match_patch.h \
    utils/diff_match_patch/diff_match_patch_controller.h \
    utils/helpers/color_helper.h \
    utils/helpers/dialog_helper.h \
    utils/helpers/extension_helper.h \
    utils/helpers/image_helper.h \
    utils/helpers/scroller_helper.h \
    utils/helpers/string_helper.h \
    utils/helpers/text_helper.h \
    utils/helpers/quotes_helper.h \
    utils/helpers/time_helper.h \
    utils/shugar.h \
    utils/tools/backup_builder.h \
    utils/tools/debouncer.h \
    utils/tools/model_index_path.h \
    utils/tools/run_once.h \
    utils/validators/email_validator.h
