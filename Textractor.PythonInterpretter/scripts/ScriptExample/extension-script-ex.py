
### v1.0.0
### This example script simply prepends a number to each sentence.
### The number increments with each sentence received by Textractor.

### This extension requires python >=3.0 to be installed on the machine.


# import any libraries/dependencies here
import os # example


# define script global variables here. These will be available from all functions as long as the script is active.
global_var_ex1 = 0
default_inc = 1
on_script_load_called = False

# This extension also initializes a logger which you can use in your own script for logging messages.
# It is a global variable called 'main_logger', which leverages python's "logging" module.
# 'main_logger' will log to the extension's python log file.
# Please try not to tamper with the logger or it could result in errors or unexpected behavior.
main_logger.info('Starting script...')


"""
This function will be called when the python script is loaded into Textractor.

This function is optional and does not need to be defined if it will not be used.
   It simply allows you to perform any actions at script load if desired.

Python scripts are loaded either
   1: When Textractor is launched, if the extension is already added.
   2: When the extension is added to Textractor.
   3: When the 'ScriptPath' extension config value is modified or a script file change is detected.
      The load will occur the next time 'process_sentence' is called by this extension, before the function starts executing.

* Pars
custom_vars:
   A map containing custom variables defined in the "ScriptCustomVars" config value in the extension's '.ini' config.
   All keys and values are strings.
return:
   Should return a bool indicating if your load actions were successful and the script should continue initializing.
   True: Continue loading script.
   False: Stop execution and do not load this script; 
         therefore, this extension will not performance any python sentence processing.

   If this function is not defined, a default one will be used which simply always returns True.
"""
def on_script_load(custom_vars):
   # do stuff here if desired
   global on_script_load_called
   on_script_load_called = True
   return True


"""
This function will be called when the python script is unloaded from Textractor.

This function is optional and does not need to be defined if it will not be used.
   It simply allows you to perform any actions at script unload if desired.

Python scripts are unloaded either
   1: When Textractor is closed, if the extension is already added.
   2: When the extension is removed from Textractor.
   3: When the 'ScriptPath' extension config value is modified or a script file change is detected.
      The unload will occur the next time 'process_sentence' is called by this extension, before the function starts executing.

* Pars
custom_vars:
   A map containing custom variables defined in the "ScriptCustomVars" config value in the extension's '.ini' config.
   All keys and values are strings.
return:
   No return value.
"""
def on_script_unload(custom_vars):
   # do stuff here if desired
   global global_var_ex1
   del global_var_ex1 # pointless action, just an example action
   main_logger.info('Unloading script...')
   return


"""
This function will be called each time text has been received by Textractor to be processed.
Textractor will display the sentence after all extensions have had a chance to process and/or modify it.
The sentence will be destroyed if it is empty.
This function may be run concurrently for each thread/hook: please make sure it's thread safe.

* Pars
sentence:
   Sentence received by Textractor, as a UTF8 string.
sentence_info:
   A map containing miscellaneous info about the sentence (see README: https://github.com/Artikash/Example-Extension).
   
   * Keys
   "current select": [int]      always 0 unless the sentence is in the text thread selected by the user.
   "process id":     [int]      process id that the sentence is coming from. 0 for console and clipboard.
   "text number":    [int]      number of the current text thread. Counts up one by one as text threads are created. 0 for console, 1 for clipboard.
   "text name":      [string]   name of the current text thread.
custom_vars:
   A map containing custom variables defined in the "ScriptCustomVars" config value in the extension's '.ini' config.
   All keys and values are strings.
return:
   Should return a utf8 string, consisting of a modified sentence for Textractor to use.
   If you do not want to make any modifications to the original sentence, then simply return the original sentence from this function.
"""
def process_sentence(sentence, sentence_info, custom_vars):
   if not sentence_info['current select']: # only process sentences on the current selected thread
      return sentence
   
   # custom variable usage example:
   # defining custom variable "inc" in the extension's .ini config allows you to control how much to increment the number by each time.
   # otherwise, a default increment of 1 will be used.
   inc = int(custom_vars["inc"]) if "inc" in custom_vars else default_inc
   
   global global_var_ex1
   global_var_ex1 += inc
   
   return str(global_var_ex1) + ': ' + sentence

main_logger.info('Done loading script...')
