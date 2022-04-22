# SpellChecker

This project was developed under the Operating Systems course at Tecnun, University of Navarra.

The project statement can be found [here](https://github.com/cbuchart/operating_systems_lectures/tree/master/src/SpellChecker).

Instructions:

SpellChecker folder:
- The project has been developed using Visual Studio 2019 Community Edition and using C++17.
- The project should be built in 64 bits(x64) Release.
- In order to make it work the following sample command line should be written:

  SpellChecker.exe words_alpha.txt test_1.txt --suggestionsOFF > test_1.html

   where words_alpha.txt is the dictionary file
   	 test_1.txt is the text input
         --suggestionsOFF is the mode -> Valid modes: --suggestionsOFF
						      --suggestionsON

 - When suggestions are OFF, the output is an html document where the incorrect words appear in red color.
 - When suggestion are ON, the output is an html document where the incorrect words that have a suggestion appear in blue and 
	the incorrect words that do not have a suggestion appear in red. If you put the mouse in the words in blue a hovering
	text should appear with the suggested correction.

 - It is parallelized to read up to 1GB files in less than 50 seconds when suggestions are off.
