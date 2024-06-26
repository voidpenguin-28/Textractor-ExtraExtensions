# Textractor.VndbCharNameMapper

The purpose of this extension is to help ensure that the Japanese names of characters are translated properly into English without needing to resort to maintaining your own manual list of name mappings.

This sometimes may be necessary to translate certain names properly since the kanji for some Japanese names have multiple different ways they can be read/interpretted. As a result, translation tools may end up translating a name incorrectly.

This extension will automatically retrieve the Japanese and English names of all known characters for a specified visual novel from https://vndb.org (popular visual novel database site). Each sentence received by Textrator will then be checked if a particular character name is found and will auto-translate it into its English equivalent, based on an appropriate name mapping.

As of v1.2.0, this also supports applying a character's gender to the text as well (if desired). Gender data is also retrieved from vndb's character database. Reference the "MappingMode" config value for more details on this feature.

![Example](img/example1.png)

The following are auto-translated/mapped:
* Full names (first & last name)
* First names
* Last names
<br>

#### How to Use (TL;DR):
1. Download extension (.xdll) and add to root Textractor directory
2. Add extension to Textractor at the right position (*after* text fixes extensions, *before* translator/display extensions)
3. Open Textractor.ini, go to extension config section, and set *VnIds* config value to the vndb visual novel identifiers of the visual novels you would like to map names for.
4. Adjust other config values as desired: [Config Values](#config-values)

<br>

**Note:**
<br>
**This extension will retrieve the names of all characters listed on vndb for the specified visual novel. *This will include all characters marked under "spoilers"*.**
<br><br>
**Therefore, if you do not want to be spoiled about the existence of certain characters, *do not look at this extension's cache file (Textractor.VndbCharNameMapper.ini)*.**
<br><br>
**In addition, if a certain character's name is not translated, then it is most likely because that character's name is not present on vndb's character list for that visual novel.**

## How to Install
1. Download the latest extension from Releases
    - https://github.com/voidpenguin-28/Textractor-ExtraExtensions/releases
2. Extract the zipped contents and place the '.xdll' extension in the root directory of the Textractor app.
	- Ensure that the chosen architecture matches the architecture of Textractor.
		- If you are using the x86 version of Textractor, then use the x86 extension
		- If you are using the x64 version of Textractor, then use the x64 extension
	- ![Copy Example](img/copy-extension.png)
3. Add the extension to Textractor
	- An extension can be added by opening Textractor, pressing the "Extensions" button, right-clicking the extensions panel, then selecting 'Add extension'.
		- Add the 'Textractor.VndbCharNameMapper.xdll' from Textractor's root directory.
	- **Once the extension is added, the positioning of the extension in the extension list is important.**
		- Make sure this extension is placed **after** any text fix extensions.
			- Ex: "Remove Repeated Characters", "Remove Repeated Phrases", etc.
		- Also make sure this extension is placed **before** any translation or display-related extensions.
			- Ex: "Google Translate", "DevTools DeepL Translate", "Extra Window", "Extra Newlines", etc.
		- An extension's order can be changed by clicking and dragging it.
		- ![Extension Order](img/extension-order.png)
4. **This extension relies on *curl.exe* to perform network requests to vndb. Ensure that curl is installed on your machine.**
	- **curl** is already installed/available on [all versions of Windows 11] & [Windows 10 ver.1803 or later].
		- Versions of curl pre-packaged with Windows can be found here: C:\Windows\System32\curl.exe
	- A quick way to test if curl is installed/available is to open a command prompt (cmd.exe), then simply type "curl".
		- If curl is found, you should get a message like "*curl: try 'curl --help' for more information*"
		- If curl is not found, you should get a message like "*'curl' is not recognized as an internal or external command,
operable program or batch file.*"
		- ![Curl Check](img/curl-check.png)
	- If you do not have curl on your machine, you can download/install it from here: https://curl.se/download.html#Win32
	- If you would like to use a portable version of curl with this extension, you can configure a custom curl path for the extension to use. Reference the [Config Values](#config-values) section of this document for further details.


## How to Use
1. Follow the instructions in the [How to Install](#how-to-install) section to ensure the extension is properly added to Textractor.
2. Once the extension has been added to Textractor, a default config section will be generated/added to the file "Textractor.ini", which you can use to configure this extension.
	- "Textractor.ini" is located in the root directory of the Textrator app and contains most app/extension config values.
	- Open "Textractor.ini" in a text editor (ex: Notepad, Notepad++)
	- The config section for this extension will match the name of the extension.
		- Ex: \[Textractor.VndbCharNameMapper\]
		- If you change the name of this extension, the generated/associated ini section name will match that name.
			- Ex: "VndbNameMapper.xdll" => \[VndbCharNameMapper\]
3. Adjust the config section for this extension in the "Textractor.ini" file accordingly.
	- Reference the [Config Values](#config-values) section for each possible value to adjust.
		- The most important config value that must be adjusted is the **VnIds** value.
	- ![Config Example](img/config-example.png)

## Config Values
Here is the list of currently supported config values for this extension.

1. **Disabled**: If set to '1', then the functionality of this extension will be disabled.
	- Default value: '0' (extension is enabled)
2. **UrlTemplate**: Specifies the core url to use for retrieving character names from vndb.
	- Default value: 'https://vndb.org/{0}/chars'
	- '{0}' indicates the position of the visual novel identifier to use.
		- Ex: If the visual novel id is 'v4', then "https://vndb.org/{0}/chars" will map to "https://vndb.org/v4/chars"
		- **Under normal cirumstances, this value should NOT be modified, or it could result in broken or unexpected behavior.**
			- This config value exists in case a future issue/cirumstance presents itself if vndb ever changed how it structures its urls/paths.
3. **VnIds**: A list of vndb visual novel identifiers, which are used to indicate which visual novels to map character names for.
	- **This is the most important config value to set. This value is blank by default, thus there is no targeted visual novel.**
	- vndb has a numeric identifier for every visual novel on the site.
		- This identifier can primarily be found in 2 places
			1. In the url for a visual novel entry: "[https://vndb.org/**v4**](https://vndb.org/v4)"
			2. The name of the primary tab of the visual novel entry. ![VnId Tab Example](img/vnid-tab-example.png)
	- Add the visual novel identifiers for all visual novels you would like to map names for.
		- Ex: If you wanted to map names for the visual novel "Clannad", you would assign the value "v4" to "VnIds"
		```ini
		[Textractor.VndbCharNameMapper]
		;;...omitted...
		VnIds=v4
		;;...omitted...
		```
		- You can map multiple visual novels by specifying multiple visual novel identifiers, separated by the character specified in the config value "VnIdDelim" (default delimiter is '|')
			- Ex: If you wanted to map names for "Clannad" or "Umineko", you would assign "v4|v24". "v4" is the id for Clannad, "v24" is the id for Umineko.
			```ini
			[Textractor.VndbCharNameMapper]
			;;...omitted...
			VnIds=v4|v24
			;;...omitted...
			```
	- As of v1.2.0, this config value also supports App Name to VnId mappings.
		- This means that if you can define key-value pairs which map a specific app to a specific VnId.
			- This can be useful if you have multiple VnIds defined but want a specific app to use a specific VnId.
		- The key would be the app name, the value would be the VnId
			- Key value pairs are delimited by an equal sign ('=')
			- The app name is the name of the executable used to run the visual novel. The easiest way to find this name is to look at the app name dropdown in Textractor.
			- ![AppName-Example](img/appname-example.png)
				- "Farthest2015" is the app name in the above example.
				- **Note: The file extension (ex: ".exe") should not be included in the app name**
			- It is okay to mix together VnIds with app name mappings and VnIds without app name mappings
				- Ex: "*ABC123=v1234|v1278|DEF456=v1188*"
		- Example: 
			```ini
			VnIds="ABC123=v1234|Farthest2015=v1278|v1188"
			VnIdDelim=|
			```
		- How mappings are applied:
			- If an app mapping is found for the current app, then the VnId tied to that app name will be used for name mappings.
			- If no app mappings if found for the current app, then all **non-mapped** VnIds will be used for name mappings.
				- Ex: If *VnIds="Farthest2014=v1234|ABC123=v1278|v1188|v1199"*, and the current app is named "MoeMoe". Then only VnIds v1188 & v1199 will be used for name mappings.
		- **Important note:** If the app name assigned to this config value contains unicode characters outside of the ASCII range (ex: Japanese characters), Textractor ends up automatically converting them to their hex equivalent when Textractor is opened or closed.
			- Ex: *VnIds="本当=v1234"* => *VnIds="\\xe6\\x9c\\xac\xe5\\xbd\\x93=v1234"*
			- Do not be alarmed, >=v1.2.1 of this extension takes this into account and will read these hex-coded values as proper unicode characters when parsing the ini file. No action is needed on your end.
4. **VnIdDelim**: The delimiter/separator to use when specifying more than one visual novel identifier in the "VnIds" config value.
	- The default value is '|', thus each identifier should be separated by a '|' . (Ex: *VnIds=v4|v24*)
	- If you changed the "VnIdDelim" value to ';', then each visual novel identifier would need to be separated by a semi-colon. (Ex: *VnIds=v4;v24*)
	```ini
	[Textractor.VndbCharNameMapper]
	;;...omitted...
	VnIds=v4;v24
	VnIdDelim=;
	;;...omitted...
	```
5. **MappingMode**: Controls how name mappings should be applied.
	- Default value: '1' (apply only name mappings)
	- Possible values:
		- **0**: Do not apply any mappings.
			- Even when using this mode, name/gender mappings will still be retrieved and cached; however, they will not be applied to the text being processed by Textractor.
		- **1**: Only apply name mappings (no gender mappings).
		- **2**: Apply name and gender mappings.
			- Gender is suffixed to the name mapping in parenthesis.
				- If gender is unknown (U), only the name will be mapped.
			- Ex: Name=Azusa, Gender=Female: "*あずさ「ち、ちがわい！」*" => "*Azusa (F)「ち、ちがわい！」*"
			- Possible gender values:
				- **M**: Male
				- **F**: Female
				- **U**: Unknown
6. **MinNameCharSize**: The minimum number of Japanese characters that must be present in a name.
	- If a character's name is less than this number, then that name will not get mapped.
	- Default value: '2'
	- Ex: If this value is set to '2' and a character has a first or last name that is only 1 JP character long (ex: 杏), then that name will not get mapped to English.
	- This config value exists to reduce potential false positives in certain cirumstances. For example, if a character's name is 杏, but a sentence uses the kanji 杏 within other non-name words, then that would result in an incorrect JP=>EN mapping.
		- If you are not concerned about false positives, then you can simply adjust this value to '1', and thus all names will get mapped regardless of length.
7. **ActiveThreadOnly**: Indicates if this extension should only map names for the currently selected thread in Textractor.
	- Default value: '1' (map current thread only)
	- If set to '0', then this extension will attempt to map names for all threads.
8. **SkipConsoleAndClipboard**: Allows you to exclude the Console and/or Clipboard threads from mapping names.
	- Default value: '1' (skip Console and Clipboard threads)
	- Possible values:
		- **0**: Do not skip Console nor Clipboard threads
		- **1**: Skip both the Console and Clipboard threads
		- **2**: Skip the Console thread (but not the Clipboard thread)
		- **3**: Skip the Clipboard thread (but not the Console thread)
	- This setting applies regardless of the value of the *ActiveThreadOnly* config key.
9. **ReloadCacheOnLaunch**: Determines whether to reload the character name mapping cache each time Textractor is launched.
	- Default value: '0' (do not reload)
	- When this extension retrieves name mappings for a visual novel from vndb, it then caches them to a cache file called "\*\*ExtensionName\*\*.ini" (ex: *Textractor.VndbCharNameMapper.ini*).
		- Name mappings are retrieved from this cache file before attempting to make a network request to vndb. This is to reduce the number of unneeded network requests your system makes to vndb.
		- By default, once name mappings for a specific visual novel are cached to this file, name mappings will always be retrieved from cache instead of from vndb.
	- Therefore, by setting this config value to '1', each time Textractor is launched, the file cache will be reloaded from vndb.
		- This can be useful for visual novel entries in vndb that may be constantly updated, such as recently released visual novels.
		- Thus this helps ensure that the list of characters is up-to-date.
10. **CustomCurlPath**: Specifies a custom directory path for where *curl.exe* is located.
	- Default value: '' (blank value indicates to use system curl path)
	- As previously stated, curl is necessary for this extension to perform network requests.
	- By default, this extension uses whatever curl path that is specified by your system's PATH variable.
	- If you would like to use curl from a different location (ex: portable version of curl), then you can specify the directory path where curl is located here.
	- Important notes:
		- Only include the **directory** path, not the curl executable itself.
			- Ex: '*C:/curl-win/*' instead of '*C:/curl-win/curl.exe*'
		- If you use backslashes in your path, **please double the backslashes**
			- Ex: '*C:\\\\curl-win\\\\*' instead of '*C:\\curl-win\\*'
			```ini
			[Textractor.VndbCharNameMapper]
			;;...omitted...
			CustomCurlPath=C:\\curl-win\\
			;;...omitted...
			```
<br>
Full config example:

```ini
[Textractor.VndbCharNameMapper]
Disabled=0
UrlTemplate=https://vndb.org/{0}/chars
VnIds=v4|v24
VnIdDelim=|
MappingMode=1
MinNameCharSize=1
ActiveThreadOnly=1
SkipConsoleAndClipboard=1
ReloadCacheOnLaunch=1
CustomCurlPath=
```