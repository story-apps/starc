TEMPLATE = subdirs

SUBDIRS = \
    comic_book_information \
    comic_book_text \
    project_information \
    screenplay_information \
    screenplay_parameters \
    screenplay_text \
    screenplay_text_structure \
    simple_text \
    simple_text_structure \
    title_page

exists (character_information/character_information.pro) {
    SUBDIRS += character_information
}

exists (location_information/location_information.pro) {
    SUBDIRS += location_information
}

exists (screenplay_statistics/screenplay_statistics.pro) {
    SUBDIRS += screenplay_statistics
}
