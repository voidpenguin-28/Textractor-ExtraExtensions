# JP to Romaji

This script leverages NLP to append the romaji form of the currently processed sentence. It relies on the **[cutlet](https://github.com/polm/cutlet)** library to do so.

**Requirements to run this script:**
1. Both the python script and the 'requirements.txt' file will be need.
2. Once these files are downloaded, add the path for the 'requirements.txt' file to the **PipRequirementsTxtPath** config value in your Textractor.ini file.
	- This is needed to installed required depedencies to run the cutlet library.

**Additional notes:**
1. The 'cutlet' library may potentially be incompatible with 32-bit python on Windows, as well as python >=3.12.
	- Therefore make sure you are running this script with a compatiable python version.
2. Since 'cutlet' leverages natural language processing (NLP) to approximate the correct jp to romaji conversion, there is no guarantee that the resulting romaji is 100% accurate.
3. Any issues/inaccuracies pertaining to the 'cutlet' library should be reported in the **['cutlet' GitHub repo](https://github.com/polm/cutlet/issues)** rather than in this repo.
