# Textractor-ExtraExtensions

This repo provides several useful Textractor extensions created by me, which are not available by default in Textactor.

[**Textractor**](https://github.com/Artikash/Textractor) is a tool used to extract text from video games and visual novels in real-time, often used to leverage translation tools to read text in other languages.


All extension releases can be found [**here**](https://github.com/voidpenguin-28/Textractor-ExtraExtensions/releases).

<br>

Here is a list of currently available extensions:
1. [**GptApiTranslate**](Textractor.GptApiTranslate): Leverages the GPT Completions API to translate text/lines, with a reasonable degree of flexibility in configuration.
2. [**VndbCharNameMapper**](Textractor.VndbCharNameMapper): Auto-maps character names from Japanese to Romaji using the character database of vndb.org for specified visual novels.
3. [**TextLogger**](Textractor.TextLogger): Provides the capability to write text from each/any thread/hook to a log file. This essentially allows you to export data/text piped by Textractor.
4. [**PythonInterpretter**](Textractor.PythonInterpretter): Allows you to write and use python scripts in Textractor. This is essentially the equivalent of a python Textractor extension. Below are python scripts currently available from this repo.
	1. **[Example Script](Textractor.PythonInterpretter/scripts/ScriptExample)**: Merely an example of how to implement a python script compatible with this extension. The code itself merely prepends an incrementing number to the currently processed sentence.
	2. **[JP to Romaji](Textractor.PythonInterpretter/scripts/JpToRomaji)**: Leverages NLP to append the romaji form of the currently processed sentence.

<br>
Documentation for each extension is available in their corresponding page in this repo.