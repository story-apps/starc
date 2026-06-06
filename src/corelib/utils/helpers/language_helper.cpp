#include "language_helper.h"


namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Internal lookup table: index = Qt5 Language ID, value = Qt6 Language ID.
// Qt5 IDs are not contiguous (gaps exist), so we use a linear search helper.
// ─────────────────────────────────────────────────────────────────────────────

struct LanguagePair {
    short qt5Id = 0;
    short qt6Id = 0;
};

// clang-format off
inline constexpr LanguagePair kLanguagePairs[] = {
    // { qt5Id, qt6Id }  // Qt5 name [ -> Qt6 name if renamed ]
    {    0,    0 },  // AnyLanguage
    {    1,    1 },  // C
    {    2,    2 },  // Abkhazian
    {    3,  220 },  // Oromo
    {    4,    3 },  // Afar
    {    5,    4 },  // Afrikaans
    {    6,    9 },  // Albanian
    {    7,   11 },  // Amharic
    {    8,   14 },  // Arabic
    {    9,   17 },  // Armenian
    {   10,   18 },  // Assamese
    {   11,   24 },  // Aymara
    {   12,   25 },  // Azerbaijani
    {   13,   32 },  // Bashkir
    {   14,   33 },  // Basque
    {   15,   30 },  // Bengali -> Bangla
    {   16,   73 },  // Dzongkha
    {   18,   39 },  // Bislama
    {   19,   43 },  // Breton
    {   20,   45 },  // Bulgarian
    {   21,   46 },  // Burmese
    {   22,   35 },  // Belarusian
    {   23,  135 },  // Khmer
    {   24,   48 },  // Catalan
    {   25,   58 },  // Chinese
    {   26,   64 },  // Corsican
    {   27,   66 },  // Croatian
    {   28,   67 },  // Czech
    {   29,   68 },  // Danish
    {   30,   72 },  // Dutch
    {   31,   75 },  // English
    {   32,   77 },  // Esperanto
    {   33,   78 },  // Estonian
    {   34,   81 },  // Faroese
    {   35,   82 },  // Fijian
    {   36,   84 },  // Finnish
    {   37,   85 },  // French
    {   38,  318 },  // WesternFrisian
    {   39,   88 },  // Gaelic
    {   40,   90 },  // Galician
    {   41,   93 },  // Georgian
    {   42,   94 },  // German
    {   43,   96 },  // Greek
    {   44,  127 },  // Greenlandic -> Kalaallisut
    {   45,   97 },  // Guarani
    {   46,   98 },  // Gujarati
    {   47,  101 },  // Hausa
    {   48,  103 },  // Hebrew
    {   49,  105 },  // Hindi
    {   50,  107 },  // Hungarian
    {   51,  108 },  // Icelandic
    {   52,  112 },  // Indonesian
    {   53,  114 },  // Interlingua
    {   54,  115 },  // Interlingue
    {   55,  116 },  // Inuktitut
    {   56,  117 },  // Inupiak -> Inupiaq
    {   57,  118 },  // Irish
    {   58,  119 },  // Italian
    {   59,  120 },  // Japanese
    {   60,  121 },  // Javanese
    {   61,  130 },  // Kannada
    {   62,  132 },  // Kashmiri
    {   63,  133 },  // Kazakh
    {   64,  138 },  // Kinyarwanda
    {   65,  150 },  // Kirghiz -> Kyrgyz
    {   66,  142 },  // Korean
    {   67,  148 },  // Kurdish
    {   68,  238 },  // Rundi
    {   69,  153 },  // Lao
    {   70,  154 },  // Latin
    {   71,  155 },  // Latvian
    {   72,  158 },  // Lingala
    {   73,  160 },  // Lithuanian
    {   74,  169 },  // Macedonian
    {   75,  174 },  // Malagasy
    {   76,  176 },  // Malay
    {   77,  175 },  // Malayalam
    {   78,  177 },  // Maltese
    {   79,  181 },  // Maori
    {   80,  183 },  // Marathi
    {   81,  184 },  // Marshallese
    {   82,  191 },  // Mongolian
    {   83,  196 },  // NauruLanguage
    {   84,  199 },  // Nepali
    {   85,  209 },  // NorwegianBokmal
    {   86,  214 },  // Occitan
    {   87,  215 },  // Oriya -> Odia
    {   88,  227 },  // Pashto
    {   89,  228 },  // Persian
    {   90,  230 },  // Polish
    {   91,  231 },  // Portuguese
    {   92,  233 },  // Punjabi
    {   93,  234 },  // Quechua
    {   94,  236 },  // Romansh
    {   95,  235 },  // Romanian
    {   96,  239 },  // Russian
    {   97,  244 },  // Samoan
    {   98,  245 },  // Sango
    {   99,  247 },  // Sanskrit
    {  100,  252 },  // Serbian
    {  101,  222 },  // Ossetic
    {  102,  268 },  // SouthernSotho
    {  103,  297 },  // Tswana
    {  104,  254 },  // Shona
    {  105,  259 },  // Sindhi
    {  106,  260 },  // Sinhala
    {  107,  274 },  // Swati
    {  108,  262 },  // Slovak
    {  109,  263 },  // Slovenian
    {  110,  265 },  // Somali
    {  111,  270 },  // Spanish
    {  112,  272 },  // Sundanese
    {  113,  273 },  // Swahili
    {  114,  275 },  // Swedish
    {  115,  249 },  // Sardinian
    {  116,  282 },  // Tajik
    {  117,  283 },  // Tamil
    {  118,  286 },  // Tatar
    {  119,  287 },  // Telugu
    {  120,  289 },  // Thai
    {  121,  290 },  // Tibetan
    {  122,  292 },  // Tigrinya
    {  123,  295 },  // Tongan
    {  124,  296 },  // Tsonga
    {  125,  298 },  // Turkish
    {  126,  299 },  // Turkmen
    {  127,  279 },  // Tahitian
    {  128,  306 },  // Uighur -> Uyghur
    {  129,  303 },  // Ukrainian
    {  130,  305 },  // Urdu
    {  131,  307 },  // Uzbek
    {  132,  310 },  // Vietnamese
    {  133,  311 },  // Volapuk
    {  134,  316 },  // Welsh
    {  135,  320 },  // Wolof
    {  136,  321 },  // Xhosa
    {  137,  323 },  // Yiddish
    {  138,  324 },  // Yoruba
    {  139,  326 },  // Zhuang
    {  140,  327 },  // Zulu
    {  141,  210 },  // NorwegianNynorsk
    {  142,   42 },  // Bosnian
    {  143,   69 },  // Divehi
    {  144,  180 },  // Manx
    {  145,   63 },  // Cornish
    {  146,    6 },  // Akan
    {  147,  141 },  // Konkani
    {  148,   89 },  // Ga
    {  149,  110 },  // Igbo
    {  150,  129 },  // Kamba
    {  151,  277 },  // Syriac
    {  152,   40 },  // Blin
    {  153,   92 },  // Geez
    {  154,  143 },  // Koro
    {  155,  257 },  // Sidamo
    {  156,   21 },  // Atsam
    {  157,  291 },  // Tigre
    {  158,  122 },  // Jju
    {  159,   86 },  // Friulian
    {  160,  309 },  // Venda
    {  161,   79 },  // Ewe
    {  162,  319 },  // Walamo -> Wolaytta
    {  163,  102 },  // Hawaiian
    {  164,  301 },  // Tyap
    {  165,  212 },  // Nyanja
    {  166,   83 },  // Filipino
    {  167,  276 },  // SwissGerman
    {  168,  255 },  // SichuanYi
    {  169,  146 },  // Kpelle
    {  170,  163 },  // LowGerman
    {  171,  269 },  // SouthNdebele
    {  172,  207 },  // NorthernSotho
    {  173,  206 },  // NorthernSami
    {  174,  284 },  // Taroko
    {  175,   99 },  // Gusii
    {  176,  281 },  // Taita
    {  177,   87 },  // Fulah
    {  178,  137 },  // Kikuyu
    {  179,  243 },  // Samburu
    {  180,  251 },  // Sena
    {  181,  208 },  // NorthNdebele
    {  182,  237 },  // Rombo
    {  183,  278 },  // Tachelhit
    {  184,  125 },  // Kabyle
    {  185,  213 },  // Nyankole
    {  186,   37 },  // Bena
    {  187,  312 },  // Vunjo
    {  188,   28 },  // Bambara
    {  189,   74 },  // Embu
    {  190,   55 },  // Cherokee
    {  191,  192 },  // Morisyen
    {  192,  173 },  // Makonde
    {  193,  152 },  // Langi
    {  194,   91 },  // Ganda
    {  195,   36 },  // Bemba
    {  196,  124 },  // Kabuverdianu
    {  197,  188 },  // Meru
    {  198,  128 },  // Kalenjin
    {  199,  195 },  // Nama
    {  200,  170 },  // Machame
    {  201,   61 },  // Colognian
    {  202,  185 },  // Masai
    {  203,  264 },  // Soga
    {  204,  168 },  // Luyia
    {  205,   20 },  // Asu
    {  206,  288 },  // Teso
    {  207,  241 },  // Saho
    {  208,  145 },  // KoyraChiini
    {  209,  240 },  // Rwa
    {  210,  166 },  // Luo
    {  211,   57 },  // Chiga
    {  212,   50 },  // CentralMoroccoTamazight -> CentralAtlasTamazight
    {  213,  144 },  // KoyraboroSenni
    {  214,  253 },  // Shambala
    {  215,   41 },  // Bodo
    {  216,   22 },  // Avaric
    {  217,   53 },  // Chamorro
    {  218,   54 },  // Chechen
    {  219,   59 },  // Church
    {  220,   60 },  // Chuvash
    {  221,   65 },  // Cree
    {  222,  100 },  // Haitian
    {  223,  104 },  // Herero
    {  224,  106 },  // HiriMotu
    {  225,  131 },  // Kanuri
    {  226,  139 },  // Komi
    {  227,  140 },  // Kongo
    {  228,  147 },  // Kwanyama -> Kuanyama
    {  229,  157 },  // Limburgish
    {  230,  164 },  // LubaKatanga
    {  231,  167 },  // Luxembourgish
    {  232,  197 },  // Navaho -> Navajo
    {  233,  198 },  // Ndonga
    {  234,  216 },  // Ojibwa
    {  235,  225 },  // Pali
    {  236,  313 },  // Walloon
    {  237,    5 },  // Aghem
    {  238,   31 },  // Basaa
    {  239,  325 },  // Zarma
    {  240,   71 },  // Duala
    {  241,  123 },  // JolaFonyi
    {  242,   80 },  // Ewondo
    {  243,   26 },  // Bafia
    {  244,  172 },  // MakhuwaMeetto
    {  245,  193 },  // Mundang
    {  246,  149 },  // Kwasio
    {  247,  211 },  // Nuer
    {  248,  242 },  // Sakha
    {  249,  246 },  // Sangu
    {  251,  285 },  // Tasawaq
    {  252,  308 },  // Vai
    {  253,  314 },  // Walser
    {  254,  322 },  // Yangben
    {  255,   23 },  // Avestan
    {  256,   19 },  // Asturian
    {  257,  202 },  // Ngomba
    {  258,  126 },  // Kako
    {  259,  189 },  // Meta
    {  260,  201 },  // Ngiemboon
    {  261,   15 },  // Aragonese
    {  262,    7 },  // Akkadian
    {  263,   12 },  // AncientEgyptian
    {  264,   13 },  // AncientGreek
    {  265,   16 },  // Aramaic
    {  266,   27 },  // Balinese
    {  267,   29 },  // Bamun
    {  268,   34 },  // BatakToba
    {  269,   44 },  // Buginese
    {  272,   52 },  // Chakma
    {  274,   62 },  // Coptic
    {  275,   70 },  // Dogri
    {  279,   95 },  // Gothic
    {  281,  113 },  // Ingush
    {  289,  178 },  // Mandingo
    {  290,  179 },  // Manipuri
    {  293,  217 },  // OldIrish
    {  294,  218 },  // OldNorse
    {  295,  219 },  // OldPersian
    {  297,  223 },  // Pahlavi
    {  299,  229 },  // Phoenician
    {  304,  248 },  // Santali
    {  305,  250 },  // Saurashtra
    {  309,  280 },  // TaiDam
    {  311,  302 },  // Ugaritic
    {  312,    8 },  // Akoose
    {  313,  151 },  // Lakota
    {  314,  271 },  // StandardMoroccanTamazight
    {  315,  182 },  // Mapuche
    {  316,   51 },  // CentralKurdish
    {  317,  162 },  // LowerSorbian
    {  318,  304 },  // UpperSorbian
    {  319,  134 },  // Kenyang
    {  320,  190 },  // Mohawk
    {  321,  204 },  // Nko
    {  322,  232 },  // Prussian
    {  323,  136 },  // Kiche
    {  324,  267 },  // SouthernSami
    {  325,  165 },  // LuleSami
    {  326,  111 },  // InariSami
    {  327,  261 },  // SkoltSami
    {  328,  315 },  // Warlpiri
    {  330,  187 },  // Mende
    {  335,  156 },  // Lezghian
    {  339,  171 },  // Maithili
    {  341,   10 },  // AmericanSignLanguage
    {  343,   38 },  // Bhojpuri
    {  345,  159 },  // LiteraryChinese
    {  346,  186 },  // Mazanderani
    {  348,  200 },  // Newari
    {  349,  205 },  // NorthernLuri
    {  350,  224 },  // Palauan
    {  351,  226 },  // Papiamento
    {  353,  293 },  // TokelauLanguage
    {  354,  294 },  // TokPisin
    {  355,  300 },  // TuvaluLanguage
    {  357,   47 },  // Cantonese
    {  358,  221 },  // Osage
    {  360,  109 },  // Ido
    {  361,  161 },  // Lojban
    {  362,  256 },  // Sicilian
    {  363,  266 },  // SouthernKurdish
    {  364,  317 },  // WesternBalochi
    {  365,   49 },  // Cebuano
    {  366,   76 },  // Erzya
    {  367,   56 },  // Chickasaw
    {  368,  194 },  // Muscogee
    {  369,  258 },  // Silesian
    {  370,  203 },  // NigerianPidgin
};
// clang-format on

inline constexpr int kPairsCount = 328;

// ─────────────────────────────────────────────────────────────────────────────
// qt5LanguageIdToQt6Id
//   Input:  a QLocale::Language integer value from a Qt5 build
//   Output: the corresponding Qt6 integer value, or -1 if not present in Qt6
// ─────────────────────────────────────────────────────────────────────────────
inline int qt5LanguageIdToQt6Id(int _qt5Id) noexcept
{
    for (int i = 0; i < kPairsCount; ++i) {
        if (kLanguagePairs[i].qt5Id == _qt5Id)
            return kLanguagePairs[i].qt6Id;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// qt6LanguageIdToQt5Id
//   Input:  a QLocale::Language integer value from a Qt6 build
//   Output: the corresponding Qt5 integer value, or -1 if not present in Qt5
// ─────────────────────────────────────────────────────────────────────────────
inline int qt6LanguageIdToQt5Id(int _qt6Id) noexcept
{
    for (int i = 0; i < kPairsCount; ++i) {
        if (kLanguagePairs[i].qt6Id == _qt6Id)
            return kLanguagePairs[i].qt5Id;
    }
    return -1;
}

} // namespace


int LanguageHelper::qt6LanguageIdToQt5Id(int _qt6Id)
{
    return ::qt6LanguageIdToQt5Id(_qt6Id);
}

int LanguageHelper::qt5LanguageIdToQt6Id(int _qt5Id)
{
    return ::qt5LanguageIdToQt6Id(_qt5Id);
}
