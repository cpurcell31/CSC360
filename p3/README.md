To Implement:
* Clean up files and separate parts
* Part 5 diskfix
* Clean up superblock information code

Program Descriptions:

Part 1: ./diskinfo test.img

Prints the information of the image file specified by test.img. This includes
superblock information, and FAT table information.

diskinfo looks at the superblock of the supplied image file then uses that
information to find FAT allocation details

Part 2: ./disklist test.img /sub_dir(optional)

Prints information of files found within the specified test.img 
and subdirectory. If no subdirectory is supplied the root directory
file information will be printed. 

disklist first finds the superblock information then analyzes the subdir string
(if provided) to navigate to the correct subdir and prints the information
of all files within the directory (including further subdirectories.

Part 3: ./diskget test.img /sub_dir/diskfile.extension localfile.extension

Copies information of specified diskfile found in test.img and subdirectory to
the local directory and given the name specified by format localfile.extension.
If localfile.extension is unavailable the program will exit, likewise, if
diskfile.extension does not exist, the program will output "File not found."

diskget finds superblock information and then analyzes the subdir string to
find the given file in the file system. If the file is found then diskget 
copies its contents into the specified localfile in the current working 
directory.

Part 4: ./diskput test.img localfile.extension /diskfile.extension

Copies information of a specified localfile to test.img's root directory.
If local file does not exist, the program will output "File not found."
While this program does work with an argument of /sub_dir/diskfile.extension
the file will not be copied into the specified subdirectory.

diskput finds superblock information and reads the localfile contents.
Then the program finds an empty space as specified by FAT and copies the
contents of localfile to test.img. Finally, the program updates the FAT to
reflect this allocation.


Part 5: ./diskfix test.img

Attempts to fix errors within the supplied test.img file. This program is only
able to handle errors of type Case 1, that is, reserved FAT blocks allocated tofiles.

diskfix finds superblock information and uses the FAT start block and block
count to find files within the FAT reserved space. Violating files are then 
moved to the next free space in both FAT and memory.

