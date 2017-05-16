/*
    dsp_tables.cpp
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"
#include "dsp.h"

//Linear frequency table for Period TO Frequency calculation:
// where Period = 7680 - Note * 64;
//	 if( Period >= 0 )
//         Frequency = table[ Period % 768 ] >> ( Period / 768 );
//       else
//         Frequency = ( table[ (7680*4+Period) % 768 ] << -( ( (7680*4+Period) / 768 ) - (7680*4)/768 ) );
// although, period is not true period in math terms;
unsigned int g_linear_freq_tab[ 768 ] =
{
    535232, 534749, 534266, 533784, 533303, 532822, 532341, 531861,
    531381, 530902, 530423, 529944, 529466, 528988, 528511, 528034,
    527558, 527082, 526607, 526131, 525657, 525183, 524709, 524236,
    523763, 523290, 522818, 522346, 521875, 521404, 520934, 520464,
    519994, 519525, 519057, 518588, 518121, 517653, 517186, 516720,
    516253, 515788, 515322, 514858, 514393, 513929, 513465, 513002,
    512539, 512077, 511615, 511154, 510692, 510232, 509771, 509312,
    508852, 508393, 507934, 507476, 507018, 506561, 506104, 505647,
    505191, 504735, 504280, 503825, 503371, 502917, 502463, 502010,
    501557, 501104, 500652, 500201, 499749, 499298, 498848, 498398,
    497948, 497499, 497050, 496602, 496154, 495706, 495259, 494812,
    494366, 493920, 493474, 493029, 492585, 492140, 491696, 491253,
    490809, 490367, 489924, 489482, 489041, 488600, 488159, 487718,
    487278, 486839, 486400, 485961, 485522, 485084, 484647, 484210,
    483773, 483336, 482900, 482465, 482029, 481595, 481160, 480726,
    480292, 479859, 479426, 478994, 478562, 478130, 477699, 477268,
    476837, 476407, 475977, 475548, 475119, 474690, 474262, 473834,
    473407, 472979, 472553, 472126, 471701, 471275, 470850, 470425,
    470001, 469577, 469153, 468730, 468307, 467884, 467462, 467041,
    466619, 466198, 465778, 465358, 464938, 464518, 464099, 463681,
    463262, 462844, 462427, 462010, 461593, 461177, 460760, 460345,
    459930, 459515, 459100, 458686, 458272, 457859, 457446, 457033,
    456621, 456209, 455797, 455386, 454975, 454565, 454155, 453745,
    453336, 452927, 452518, 452110, 451702, 451294, 450887, 450481,
    450074, 449668, 449262, 448857, 448452, 448048, 447644, 447240,
    446836, 446433, 446030, 445628, 445226, 444824, 444423, 444022,
    443622, 443221, 442821, 442422, 442023, 441624, 441226, 440828,
    440430, 440033, 439636, 439239, 438843, 438447, 438051, 437656,
    437261, 436867, 436473, 436079, 435686, 435293, 434900, 434508,
    434116, 433724, 433333, 432942, 432551, 432161, 431771, 431382,
    430992, 430604, 430215, 429827, 429439, 429052, 428665, 428278,
    427892, 427506, 427120, 426735, 426350, 425965, 425581, 425197,
    424813, 424430, 424047, 423665, 423283, 422901, 422519, 422138,
    421757, 421377, 420997, 420617, 420237, 419858, 419479, 419101,
    418723, 418345, 417968, 417591, 417214, 416838, 416462, 416086,
    415711, 415336, 414961, 414586, 414212, 413839, 413465, 413092,
    412720, 412347, 411975, 411604, 411232, 410862, 410491, 410121,
    409751, 409381, 409012, 408643, 408274, 407906, 407538, 407170,
    406803, 406436, 406069, 405703, 405337, 404971, 404606, 404241,
    403876, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
    400970, 400609, 400247, 399886, 399525, 399165, 398805, 398445,
    398086, 397727, 397368, 397009, 396651, 396293, 395936, 395579,
    395222, 394865, 394509, 394153, 393798, 393442, 393087, 392733,
    392378, 392024, 391671, 391317, 390964, 390612, 390259, 389907,
    389556, 389204, 388853, 388502, 388152, 387802, 387452, 387102,
    386753, 386404, 386056, 385707, 385359, 385012, 384664, 384317,
    383971, 383624, 383278, 382932, 382587, 382242, 381897, 381552,
    381208, 380864, 380521, 380177, 379834, 379492, 379149, 378807,
    378466, 378124, 377783, 377442, 377102, 376762, 376422, 376082,
    375743, 375404, 375065, 374727, 374389, 374051, 373714, 373377,
    373040, 372703, 372367, 372031, 371695, 371360, 371025, 370690,
    370356, 370022, 369688, 369355, 369021, 368688, 368356, 368023,
    367691, 367360, 367028, 366697, 366366, 366036, 365706, 365376,
    365046, 364717, 364388, 364059, 363731, 363403, 363075, 362747,
    362420, 362093, 361766, 361440, 361114, 360788, 360463, 360137,
    359813, 359488, 359164, 358840, 358516, 358193, 357869, 357547,
    357224, 356902, 356580, 356258, 355937, 355616, 355295, 354974,
    354654, 354334, 354014, 353695, 353376, 353057, 352739, 352420,
    352103, 351785, 351468, 351150, 350834, 350517, 350201, 349885,
    349569, 349254, 348939, 348624, 348310, 347995, 347682, 347368,
    347055, 346741, 346429, 346116, 345804, 345492, 345180, 344869,
    344558, 344247, 343936, 343626, 343316, 343006, 342697, 342388,
    342079, 341770, 341462, 341154, 340846, 340539, 340231, 339924,
    339618, 339311, 339005, 338700, 338394, 338089, 337784, 337479,
    337175, 336870, 336566, 336263, 335959, 335656, 335354, 335051,
    334749, 334447, 334145, 333844, 333542, 333242, 332941, 332641,
    332341, 332041, 331741, 331442, 331143, 330844, 330546, 330247,
    329950, 329652, 329355, 329057, 328761, 328464, 328168, 327872,
    327576, 327280, 326985, 326690, 326395, 326101, 325807, 325513,
    325219, 324926, 324633, 324340, 324047, 323755, 323463, 323171,
    322879, 322588, 322297, 322006, 321716, 321426, 321136, 320846,
    320557, 320267, 319978, 319690, 319401, 319113, 318825, 318538,
    318250, 317963, 317676, 317390, 317103, 316817, 316532, 316246,
    315961, 315676, 315391, 315106, 314822, 314538, 314254, 313971,
    313688, 313405, 313122, 312839, 312557, 312275, 311994, 311712,
    311431, 311150, 310869, 310589, 310309, 310029, 309749, 309470,
    309190, 308911, 308633, 308354, 308076, 307798, 307521, 307243,
    306966, 306689, 306412, 306136, 305860, 305584, 305308, 305033,
    304758, 304483, 304208, 303934, 303659, 303385, 303112, 302838,
    302565, 302292, 302019, 301747, 301475, 301203, 300931, 300660,
    300388, 300117, 299847, 299576, 299306, 299036, 298766, 298497,
    298227, 297958, 297689, 297421, 297153, 296884, 296617, 296349,
    296082, 295815, 295548, 295281, 295015, 294749, 294483, 294217,
    293952, 293686, 293421, 293157, 292892, 292628, 292364, 292100,
    291837, 291574, 291311, 291048, 290785, 290523, 290261, 289999,
    289737, 289476, 289215, 288954, 288693, 288433, 288173, 287913,
    287653, 287393, 287134, 286875, 286616, 286358, 286099, 285841,
    285583, 285326, 285068, 284811, 284554, 284298, 284041, 283785,
    283529, 283273, 283017, 282762, 282507, 282252, 281998, 281743,
    281489, 281235, 280981, 280728, 280475, 280222, 279969, 279716,
    279464, 279212, 278960, 278708, 278457, 278206, 277955, 277704,
    277453, 277203, 276953, 276703, 276453, 276204, 275955, 275706,
    275457, 275209, 274960, 274712, 274465, 274217, 273970, 273722,
    273476, 273229, 272982, 272736, 272490, 272244, 271999, 271753,
    271508, 271263, 271018, 270774, 270530, 270286, 270042, 269798,
    269555, 269312, 269069, 268826, 268583, 268341, 268099, 267857
};

//Half of sinus:
unsigned char g_hsin_tab[ 256 ] =
{
    0, 3, 6, 9, 12, 15, 18, 21, 25, 28, 31, 34, 37, 40, 43, 46,
    49, 52, 56, 59, 62, 65, 68, 71, 74, 77, 80, 83, 86, 89, 92, 95, 
    97, 100, 103, 106, 109, 112, 115, 117, 120, 123, 126, 128, 131, 134, 136, 139, 
    142, 144, 147, 149, 152, 154, 157, 159, 162, 164, 167, 169, 171, 174, 176, 178, 
    180, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 
    212, 214, 216, 217, 219, 221, 222, 224, 225, 227, 228, 229, 231, 232, 233, 235, 
    236, 237, 238, 239, 240, 242, 243, 243, 244, 245, 246, 247, 248, 249, 249, 250, 
    251, 251, 252, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 252, 251, 
    251, 250, 249, 249, 248, 247, 246, 245, 245, 244, 243, 242, 241, 240, 238, 237, 
    236, 235, 234, 232, 231, 230, 228, 227, 225, 224, 222, 221, 219, 218, 216, 214, 
    213, 211, 209, 207, 205, 203, 201, 200, 198, 196, 194, 191, 189, 187, 185, 183, 
    181, 179, 176, 174, 172, 169, 167, 165, 162, 160, 157, 155, 152, 150, 147, 145, 
    142, 139, 137, 134, 131, 129, 126, 123, 120, 118, 115, 112, 109, 106, 104, 101, 
    98, 95, 92, 89, 86, 83, 80, 77, 74, 71, 68, 65, 62, 59, 56, 53, 
    50, 47, 44, 41, 37, 34, 31, 28, 25, 22, 19, 16, 12, 9, 6, 3
};
