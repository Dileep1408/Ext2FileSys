#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    // ============================================================
    // 1. CREATE FILESYSTEM OBJECT
    // ============================================================

    FileSystem fileSystem;

    // Path to the ext2 disk image
    string imagePath = "Artifacts/disk-backpup.img";


    // ============================================================
    // 2. OPEN DISK IMAGE
    // ============================================================

    if (!fileSystem.openImage(imagePath))
    {
        cout << "Failed to open disk image." << endl;
        return 1;
    }

    cout << "Disk image opened successfully." << endl;


    // ============================================================
    // 3. READ SUPERBLOCK
    // ============================================================

    if (!fileSystem.readSuperblock())
    {
        cout << "Failed to read superblock." << endl;
        fileSystem.closeImage();
        return 1;
    }

    cout << "Superblock read successfully." << endl;


    // ============================================================
    // 4. READ BLOCK GROUP DESCRIPTORS
    // ============================================================

    if (!fileSystem.readGroupDescriptors())
    {
        cout << "Failed to read block group descriptors." << endl;
        fileSystem.closeImage();
        return 1;
    }

    cout << "Block group descriptors read successfully." << endl;


    // ============================================================
    // 5. DISPLAY FILESYSTEM INFORMATION
    // ============================================================

    cout << endl;
    cout << "========================================" << endl;
    cout << "        EXT2 FILESYSTEM INFORMATION     " << endl;
    cout << "========================================" << endl;

    fileSystem.printSuperblock();

    cout << endl;

    fileSystem.printGroupDescriptors();


    // ============================================================
    // 6. CREATE DIRECTORY AND FILE OBJECTS
    // ============================================================

    Directory directory(fileSystem);
    File file(fileSystem);


    // ============================================================
    // 7. DISPLAY COMPLETE DIRECTORY TREE
    // ============================================================

    cout << endl;
    cout << "========================================" << endl;
    cout << "        FILESYSTEM DIRECTORY TREE       " << endl;
    cout << "========================================" << endl;

    if (!directory.traverseFromRoot())
    {
        cout << "Failed to traverse filesystem." << endl;
    }


    // ============================================================
    // 8. MENU
    // ============================================================

    while (true)
    {
        cout << endl;
        cout << "========================================" << endl;
        cout << "                MENU                    " << endl;
        cout << "========================================" << endl;

        cout << "1. Display file contents" << endl;
        cout << "2. Overwrite file" << endl;
        cout << "3. Append to file" << endl;
        cout << "4. Display filesystem information" << endl;
        cout << "5. Display directory tree" << endl;
        cout << "6. Exit" << endl;

        cout << endl;
        cout << "Enter your choice: ";

        int choice;
        cin >> choice;

        // Remove the newline left by cin
        cin.ignore();


        // ========================================================
        // OPTION 1: DISPLAY FILE
        // ========================================================

        if (choice == 1)
        {
            string path;

            cout << "Enter file path: ";
            getline(cin, path);

            cout << endl;

            if (!file.displayFile(path))
            {
                cout << "Failed to display file." << endl;
            }
        }


        // ========================================================
        // OPTION 2: OVERWRITE FILE
        // ========================================================

        else if (choice == 2)
        {
            string path;
            string content;

            cout << "Enter file path: ";
            getline(cin, path);

            cout << "Enter new content: ";
            getline(cin, content);

            cout << endl;

            if (file.overwriteFile(path, content))
            {
                cout << "File overwritten successfully." << endl;
            }
            else
            {
                cout << "Failed to overwrite file." << endl;
            }
        }


        // ========================================================
        // OPTION 3: APPEND TO FILE
        // ========================================================

        else if (choice == 3)
        {
            string path;
            string content;

            cout << "Enter file path: ";
            getline(cin, path);

            cout << "Enter content to append: ";
            getline(cin, content);

            cout << endl;

            if (file.appendToFile(path, content))
            {
                cout << "Content appended successfully." << endl;
            }
            else
            {
                cout << "Failed to append to file." << endl;
            }
        }


        // ========================================================
        // OPTION 4: DISPLAY FILESYSTEM INFORMATION
        // ========================================================

        else if (choice == 4)
        {
            cout << endl;

            cout << "========================================" << endl;
            cout << "        EXT2 FILESYSTEM INFORMATION     " << endl;
            cout << "========================================" << endl;

            fileSystem.printSuperblock();

            cout << endl;

            fileSystem.printGroupDescriptors();
        }


        // ========================================================
        // OPTION 5: DISPLAY DIRECTORY TREE
        // ========================================================

        else if (choice == 5)
        {
            cout << endl;

            cout << "========================================" << endl;
            cout << "        FILESYSTEM DIRECTORY TREE       " << endl;
            cout << "========================================" << endl;

            if (!directory.traverseFromRoot())
            {
                cout << "Failed to traverse filesystem." << endl;
            }
        }


        // ========================================================
        // OPTION 6: EXIT
        // ========================================================

        else if (choice == 6)
        {
            cout << "Closing filesystem..." << endl;

            fileSystem.closeImage();

            cout << "Program terminated." << endl;

            return 0;
        }


        // ========================================================
        // INVALID OPTION
        // ========================================================

        else
        {
            cout << "Invalid choice. Please try again." << endl;
        }
    }


    return 0;
}