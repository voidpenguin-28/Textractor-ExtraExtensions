
### v1.0.0
### NOTE: The 'cutlet' library may potentially be incompatible with 32-bit python on Windows, as well as python >=3.12.

import cutlet

zero_width_space_ch = '\u200b'
katsu = cutlet.Cutlet()

def process_sentence(sentence, sentence_info, custom_vars):
    split_sents = sentence.split(zero_width_space_ch)
    romaji_sent = katsu.romaji(split_sents[0])
    split_sents.insert(1, '\n' + romaji_sent)
    return zero_width_space_ch.join(split_sents)
