TEMPLATE = subdirs

SUBDIRS = \
    project_information \
    screenplay_information \
    screenplay_parameters \
    screenplay_text \
    screenplay_text_structure \
    simple_text \
    simple_text_structure

exists (character_information/character_information.pro) {
    SUBDIRS += character_information
}

exists (location_information/location_information.pro) {
    SUBDIRS += location_information
}

exists (screenplay_statistics/screenplay_statistics.pro) {
    SUBDIRS += screenplay_statistics
}
