// General MIDI (GM) Sound Constants
// Use these with selectProgram() function
// Note: These are 1-based numbers as per GM standard

// PIANO (1-8)
#define GM_ACOUSTIC_PIANO       1
#define GM_BRIGHT_PIANO         2
#define GM_ELECTRIC_GRAND       3
#define GM_HONKY_TONK_PIANO     4
#define GM_ELECTRIC_PIANO_1     5
#define GM_ELECTRIC_PIANO_2     6
#define GM_HARPSICHORD          7
#define GM_CLAVINET             8

// CHROMATIC PERCUSSION (9-16)
#define GM_CELESTA              9
#define GM_GLOCKENSPIEL         10
#define GM_MUSIC_BOX            11
#define GM_VIBRAPHONE           12
#define GM_MARIMBA              13
#define GM_XYLOPHONE            14
#define GM_TUBULAR_BELLS        15
#define GM_DULCIMER             16

// ORGAN (17-24)
#define GM_DRAWBAR_ORGAN        17
#define GM_PERCUSSIVE_ORGAN     18
#define GM_ROCK_ORGAN           19
#define GM_CHURCH_ORGAN         20
#define GM_REED_ORGAN           21
#define GM_ACCORDION            22
#define GM_HARMONICA            23
#define GM_TANGO_ACCORDION      24

// GUITAR (25-32)
#define GM_NYLON_GUITAR         25
#define GM_STEEL_GUITAR         26
#define GM_JAZZ_GUITAR          27
#define GM_CLEAN_GUITAR         28
#define GM_MUTED_GUITAR         29
#define GM_OVERDRIVE_GUITAR     30
#define GM_DISTORTION_GUITAR    31
#define GM_GUITAR_HARMONICS     32

// BASS (33-40)
#define GM_ACOUSTIC_BASS        33
#define GM_FINGERED_BASS        34
#define GM_PICKED_BASS          35
#define GM_FRETLESS_BASS        36
#define GM_SLAP_BASS_1          37
#define GM_SLAP_BASS_2          38
#define GM_SYNTH_BASS_1         39
#define GM_SYNTH_BASS_2         40

// STRINGS (41-48)
#define GM_VIOLIN               41
#define GM_VIOLA                42
#define GM_CELLO                43
#define GM_CONTRABASS           44
#define GM_TREMOLO_STRINGS      45
#define GM_PIZZICATO_STRINGS    46
#define GM_ORCHESTRAL_HARP      47
#define GM_TIMPANI              48

// ENSEMBLE (49-56)
#define GM_STRING_ENSEMBLE_1    49
#define GM_STRING_ENSEMBLE_2    50
#define GM_SYNTH_STRINGS_1      51
#define GM_SYNTH_STRINGS_2      52
#define GM_CHOIR_AAHS           53
#define GM_VOICE_OOHS           54
#define GM_SYNTH_VOICE          55
#define GM_ORCHESTRA_HIT        56

// BRASS (57-64)
#define GM_TRUMPET              57
#define GM_TROMBONE             58
#define GM_TUBA                 59
#define GM_MUTED_TRUMPET        60
#define GM_FRENCH_HORN          61
#define GM_BRASS_SECTION        62
#define GM_SYNTH_BRASS_1        63
#define GM_SYNTH_BRASS_2        64

// REED (65-72)
#define GM_SOPRANO_SAX          65
#define GM_ALTO_SAX             66
#define GM_TENOR_SAX            67
#define GM_BARITONE_SAX         68
#define GM_OBOE                 69
#define GM_ENGLISH_HORN         70
#define GM_BASSOON              71
#define GM_CLARINET             72

// PIPE (73-80)
#define GM_PICCOLO              73
#define GM_FLUTE                74
#define GM_RECORDER             75
#define GM_PAN_FLUTE            76
#define GM_BLOWN_BOTTLE         77
#define GM_SHAKUHACHI           78
#define GM_WHISTLE              79
#define GM_OCARINA              80

// SYNTH LEAD (81-88)
#define GM_SQUARE_LEAD          81
#define GM_SAWTOOTH_LEAD        82
#define GM_CALLIOPE_LEAD        83
#define GM_CHIFF_LEAD           84
#define GM_CHARANG_LEAD         85
#define GM_VOICE_LEAD           86
#define GM_FIFTHS_LEAD          87
#define GM_BASS_LEAD            88

// SYNTH PAD (89-96)
#define GM_NEW_AGE_PAD          89
#define GM_WARM_PAD             90
#define GM_POLYSYNTH_PAD        91
#define GM_CHOIR_PAD            92
#define GM_BOWED_PAD            93
#define GM_METALLIC_PAD         94
#define GM_HALO_PAD             95
#define GM_SWEEP_PAD            96

// SYNTH EFFECTS (97-104)
#define GM_RAIN_FX              97
#define GM_SOUNDTRACK_FX        98
#define GM_CRYSTAL_FX           99
#define GM_ATMOSPHERE_FX        100
#define GM_BRIGHTNESS_FX        101
#define GM_GOBLINS_FX           102
#define GM_ECHOES_FX            103
#define GM_SCI_FI_FX            104

// ETHNIC (105-112)
#define GM_SITAR                105
#define GM_BANJO                106
#define GM_SHAMISEN             107
#define GM_KOTO                 108
#define GM_KALIMBA              109
#define GM_BAGPIPE              110
#define GM_FIDDLE               111
#define GM_SHANAI               112

// PERCUSSIVE (113-120)
#define GM_TINKLE_BELL          113
#define GM_AGOGO                114
#define GM_STEEL_DRUMS          115
#define GM_WOODBLOCK            116
#define GM_TAIKO_DRUM           117
#define GM_MELODIC_TOM          118
#define GM_SYNTH_DRUM           119
#define GM_REVERSE_CYMBAL       120

// SOUND EFFECTS (121-128)
#define GM_GUITAR_FRET_NOISE    121
#define GM_BREATH_NOISE         122
#define GM_SEASHORE             123
#define GM_BIRD_TWEET           124
#define GM_TELEPHONE_RING       125
#define GM_HELICOPTER           126
#define GM_APPLAUSE             127
#define GM_GUNSHOT              128

// Commonly used sustained sounds (good for long notes)
#define GM_SUSTAINED_STRINGS    GM_STRING_ENSEMBLE_1
#define GM_SUSTAINED_ORGAN      GM_CHURCH_ORGAN
#define GM_SUSTAINED_PAD        GM_NEW_AGE_PAD
#define GM_SUSTAINED_BRASS      GM_BRASS_SECTION
#define GM_SUSTAINED_CHOIR      GM_CHOIR_AAHS
