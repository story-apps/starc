TEMPLATE = subdirs

SUBDIRS = \
    audioplay_information \
    audioplay_parameters \
    audioplay_text \
    audioplay_text_structure \
    comic_book_information \
    comic_book_parameters \
    comic_book_text \
    comic_book_text_structure \
    project_information \
    screenplay_information \
    screenplay_parameters \
    screenplay_treatment \
    screenplay_treatment_structure \
    screenplay_text \
    screenplay_text_structure \
    simple_text \
    simple_text_structure \
    stageplay_information \
    stageplay_parameters \
    stageplay_text \
    stageplay_text_structure \
    title_page \
    recycle_bin

exists (project_collaborators/project_collaborators.pro) {
    SUBDIRS += project_collaborators
}

exists (characters_relations/characters_relations.pro) {
    SUBDIRS += characters_relations
}

exists (character_information/character_information.pro) {
    SUBDIRS += character_information
}

exists (locations_map/locations_map.pro) {
    SUBDIRS += locations_map
}

exists (location_information/location_information.pro) {
    SUBDIRS += location_information
}

exists (screenplay_statistics/screenplay_statistics.pro) {
    SUBDIRS += screenplay_statistics
}

exists (comic_book_statistics/comic_book_statistics.pro) {
    SUBDIRS += comic_book_statistics
}

exists (audioplay_statistics/audioplay_statistics.pro) {
    SUBDIRS += audioplay_statistics
}

exists (stageplay_statistics/stageplay_statistics.pro) {
    SUBDIRS += stageplay_statistics
}
