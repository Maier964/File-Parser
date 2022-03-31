#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>



#define USAGE "USAGE:\n\n./a1 <OPTIONS> <PARAMETERS>\n ~ variant \n \
~ list [RECURSIVE]? <filtering_options> path=<PATH> \n ~ parse path=<PATH>\n \
~ extract path=<PATH> section=<SECTION_NR> line=<LINE_NR> \n \
~ findall path=<DIR_PATH>"

// This preprocessing instruction doesn't do anything from a logical perspective,
// it just aids towards program visibility (optional parameters are marked)
#define OPTIONAL 

#define LOG_ERROR(format, ...) \
printf("ERROR:" "[" __TIME__ "] " __FILE__ "] [" ":" "%d" "]: " format "\n", __LINE__, __VA_ARGS__);

#define TRUE 1
#define FALSE 0

#define MAX_PATH_SIZE 4069
#define SF_SECTION_HEADER_SIZE 25

static _Bool listOk = FALSE; 

typedef enum _HSTATUS{
    STATUS_SUCCESS = 0,
    STATUS_INVALID_PARAMETERS = -2,
    STATUS_INVALID_MEMORY = -3,
    STATUS_FAILED = -1,
    STATUS_INVALID_STATUS = 1,
    STATUS_NOT_SF = -4,
    STATUS_INVALID_COMMAND = -5
} STATUS, *PSTATUS;

#pragma pack(1)

typedef struct _HEADER_SECTION
{
    int sectOffset;
    int sectSize;
    char* sectName;
    unsigned short sectType:8;
}SF_HEADER_SECTION, *PSF_HEADER_SECTION;


typedef struct _SF_HEADER{
    PSF_HEADER_SECTION sections;
    unsigned short version:8;
    unsigned short noOfSections:4;
}SF_HEADER, *PSF_HEADER;

STATUS list( char*, OPTIONAL _Bool, OPTIONAL char*, OPTIONAL _Bool, OPTIONAL _Bool );
STATUS parse( const char*, OPTIONAL _Bool, OPTIONAL _Bool, OPTIONAL PSF_HEADER );
STATUS extract(const char*, const char*, const char* );
STATUS countNewlines( PSF_HEADER, char*, int* );
void freeHeader( PSF_HEADER*  );

int main(int argc, char **argv){

    STATUS controlStatus = STATUS_INVALID_STATUS;

    if(argc >= 2){

        // Variant command
        if( !(strcmp(argv[1], "variant") ) )
        {
            //Assure that no other argument is passed next to variant
            if( argc > 2 ){
                printf(USAGE);
                _exit(-1);
            }

            setbuf(stdout,NULL);
            
            printf("85967");

            _exit(0);

        }

        // List command
        if( !( strcmp(argv[1], "list") ) )
        {
            // Path is always last, only options can juggle 
            _Bool optionalRecursive = 0;
            char* optionalEndsWith = NULL;
            _Bool optionalHasWriteRights = 0;
            int list_arg_count = 2; // source file name and "list"

            if ( strstr( argv[argc - 1] , "path=" ) )
            {
                list_arg_count++;
                for ( int i = 1; i < argc - 1; i++ )
                {
                    if ( strstr( argv[i], "recursive")  && 
                     !( strcmp("recursive", (strstr( argv[i], "recursive")) ) ) )
                     {
                        optionalRecursive = 1;
                        list_arg_count++;
                     }

                    if ( strstr ( argv[i], "name_ends_with=") )
                    {
                        optionalEndsWith = (char*)malloc(256);
                        strcpy(optionalEndsWith, argv[i] + 15); 
                        list_arg_count++;
                    }

                    if ( strstr ( argv[i], "has_perm_write") &&
                    !( strcmp("has_perm_write", strstr ( argv[i], "has_perm_write") ) ) )
                    {
                        optionalHasWriteRights = 1;
                        list_arg_count++;
                    }
                        
                }

                // Simple form of input sanitization
                if ( list_arg_count != argc )
                {
                    LOG_ERROR("ERROR: Invalid options for \"list\", GLE=%x", errno);
                    return STATUS_INVALID_PARAMETERS;
                }

                controlStatus = list( argv[ argc - 1  ] + 5, optionalRecursive, optionalEndsWith, optionalHasWriteRights, FALSE);

                free(optionalEndsWith);

                if ( controlStatus )
                {
                    printf("ERROR: INVALID RETURN VALUE");
                    exit( STATUS_FAILED );
                }


                exit( STATUS_SUCCESS );
            }
            else 
                printf("Invalid path for list command, GLE = %x\n",errno);

            exit( STATUS_INVALID_PARAMETERS );
        }

        // Parse command
        if ( !strcmp(argv[1], "parse") )
        {

            if ( strstr(argv[ argc - 1 ], "path=") )
            {
                controlStatus = parse( argv[ argc - 1 ] + 5, TRUE, FALSE, NULL ); 

                if ( controlStatus )
                {
                    exit ( STATUS_NOT_SF );
                }  

                exit( STATUS_SUCCESS );
            }
            else
            { 
                printf("Invalid options for \"parse\", GLE = %x\n",errno);
                exit( STATUS_INVALID_COMMAND );
            }
        }

        // Extract command
        if ( !strcmp(argv[1], "extract") )
        {
            const char* path, *section, *line;
            int expectedArgc = 0;

            // Iterate over every argument
            for ( int i = 2; i <= argc - 1; i++ )
            {
                if ( strstr(argv[ i ], "path=") )
                {
                    path = argv[i] + 5;
                    expectedArgc++;
                }

            

                if ( strstr(argv[ i ], "section=") )
                {
                    section = argv[i] + 8;
                    expectedArgc++;
                }

                if ( strstr(argv[ i ], "line=") )
                {
                    line = argv[i] + 5;
                    expectedArgc++;
                }                                
                
            }

            // All arguments must be present for the command to be valid ( and no other arguments )
            if ( expectedArgc != 3 || NULL == section || NULL == path || NULL == line  )
            {
                LOG_ERROR("Invalid args for \"extract\", GLE: %x ", errno);
                exit( STATUS_INVALID_COMMAND );
            }

            controlStatus = extract( path, section, line);

            if ( controlStatus )
            {   
                exit( STATUS_FAILED );
            }

            exit(STATUS_SUCCESS);
        }

        if ( !( strcmp( argv[1], "findall" ) ) )
        {
            if ( strstr(argv[ argc - 1 ], "path=" ) )
            {
                setbuf(stdout, NULL);
                // Enchanted list

                controlStatus = list( argv[ argc - 1 ] + 5,TRUE, NULL, NULL, TRUE);

                if ( controlStatus )
                {
                    exit ( STATUS_FAILED );
                }

                exit( STATUS_SUCCESS );
            }
            else
            {
                LOG_ERROR("Invalid options for \"findall\", GLE=%x", errno);
                exit( STATUS_INVALID_COMMAND );
            }
        }
    }
    else
    {
        printf(USAGE);
        _exit( STATUS_INVALID_PARAMETERS );
    }
    return 0;
}

/**
 * @brief Lists files and directories.
 * @param PATH Path to the starting directory
 * @param RECURSIVE List recursively
 * @param endsWith Only list files that end with a certain pattern
 * @param hasWrite Only list files that the user has permission to write to
 * @param checkSFAndLines Filter listing based on specific conditions (more details in the assingment)
 * @return STATUS variable with a corresponding meaning
 */
STATUS list( char* PATH, OPTIONAL _Bool RECURSIVE, OPTIONAL char* endsWith, OPTIONAL _Bool hasWrite, OPTIONAL _Bool checkSFAndLines )
{

    DIR* workingDir;
    char* nextDirName = NULL;
    char* fileRelativePath = NULL;
    struct dirent *dirIterator;
    struct stat fileStatus; 
    PSF_HEADER header = NULL;
    int lineCountDesired;

    if ( NULL == PATH )
    {
        return STATUS_INVALID_PARAMETERS;
    }


    // Opens buffer to directory structure
    workingDir = opendir(PATH);
    
    if ( workingDir == 0 )
    {
        printf("ERROR\ninvalid directory path");
        return STATUS_FAILED;
    }

    nextDirName = (char*)malloc(255); // Maximum filename size in Unix-based systems
    fileRelativePath = (char*)malloc(MAX_PATH_SIZE);

    if ( NULL == nextDirName || NULL == fileRelativePath )
    {
        closedir(workingDir);
        return STATUS_INVALID_MEMORY;
    }

    if( !RECURSIVE )
    {
        // Iterative approach, no subdirectory listing.
        printf("SUCCESS\n");

        while( (dirIterator = readdir(workingDir)) != 0 )
        {

            // Work with relative paths
            snprintf(fileRelativePath, MAX_PATH_SIZE, "%s/%s",PATH,dirIterator->d_name);

            // Ignore relative directories
            if ( !strcmp(dirIterator->d_name, ".") || !(strcmp(dirIterator->d_name, "..")) )
                continue;

            // endsWith option is enabled
            if ( endsWith )
            {
                if ( strlen(endsWith) <= strlen(dirIterator->d_name) )
                {
                    if( strstr(  dirIterator->d_name + (strlen(dirIterator->d_name) - strlen(endsWith)) , endsWith ) != NULL )
                        printf("%s\n", fileRelativePath);
                }
                continue;
            }

            // hasWrite is enabled
            if ( hasWrite )
            {
                // stat returns negative value on failure
                if ( stat(fileRelativePath, &fileStatus) < STATUS_SUCCESS )
                {
                    LOG_ERROR("stat cannot recognise file %s", fileRelativePath);
                }

                // check for writing permissions
                ( fileStatus.st_mode & S_IWUSR ) ? printf("%s\n", fileRelativePath) : 0;
                continue;
            }
            printf("%s\n", fileRelativePath);
        }
    }   
    else
    {
        while( (dirIterator = readdir(workingDir)) != 0 )
        {
            // Work with relative paths
            snprintf(fileRelativePath, MAX_PATH_SIZE, "%s/%s",PATH,dirIterator->d_name);

            // Tiganeala to print a SUCCESS message only at the beginning of the recursion
            if (!listOk) 
            {
                listOk = TRUE;
                puts("SUCCESS");
            }

            // Ignore relative directories
            if ( !strcmp(dirIterator->d_name, ".") || !(strcmp(dirIterator->d_name, "..")) )
                continue;

            if ( stat(fileRelativePath, &fileStatus) < STATUS_SUCCESS )
            {
                LOG_ERROR("stat cannot recognise file %s", fileRelativePath);
            }


            if ( S_ISDIR(fileStatus.st_mode) && strcmp(dirIterator->d_name, ".") && strcmp(dirIterator->d_name, "..") )
            {

                if ( !checkSFAndLines ) printf("%s\n", fileRelativePath);
                list(fileRelativePath, RECURSIVE, endsWith, hasWrite, checkSFAndLines);
                continue;
            }



            if ( endsWith )
            {
                if ( strlen(endsWith) <= strlen(dirIterator->d_name) )
                {
                    if( strstr(  dirIterator->d_name + (strlen(dirIterator->d_name) - strlen(endsWith)) , endsWith ) != NULL )
                        printf("%s\n", fileRelativePath);
                }
                continue;
            }

            // hasWrite is enabled
            if ( hasWrite )
            {

                // check for writing permissions
                ( fileStatus.st_mode & S_IWUSR ) ? printf("%s\n", fileRelativePath) : 0;
                continue;
            }

            if ( checkSFAndLines )
            {
                // Allocate header
                header = (PSF_HEADER)malloc(sizeof( SF_HEADER ));

                if ( parse( fileRelativePath, FALSE, TRUE, header ) != 0 )
                {
                    // Not a SF file
                    continue;
                }
                else
                {
                    // SF file, header is initialised corrrectly
                    // Iterate trough all header sections
                    lineCountDesired = 0;
                    
                    countNewlines( header, fileRelativePath, &lineCountDesired );

                    if ( lineCountDesired >= 2 )
                        printf("%s\n", fileRelativePath);

                    freeHeader(&header);
                }
            }
            else printf("%s\n", fileRelativePath);
        }
    }
    free(fileRelativePath);
    free(nextDirName);
    closedir(workingDir);
    return STATUS_SUCCESS;
}



/**
 * @brief Checks if file is SF-compilant.
 * @param filePath Path to the desired file
 * @param verbose Print output information
 * @param keepHeader Don't free header structure. Instead, associate headerToExtract param to the memory location
 * @param headerToExtract Pointer to the allocated structure in case keepHeader is true
 * @warning if keepHeader is used, headerToExtract must be allocated prior!
 * @return STATUS variable with a corresponding meaning
 */
STATUS parse( const char* filePath, OPTIONAL _Bool verbose, OPTIONAL _Bool keepHeader, OPTIONAL PSF_HEADER headerToExtract )
{
    char* buffer = malloc(SF_SECTION_HEADER_SIZE + 2); // size of a section header + delimiter
    int fd;
    unsigned short* headerSize = (unsigned short*)malloc( sizeof(unsigned short) );
    PSF_HEADER header;
    int cnt=0;

    if( keepHeader )
    {
        // Assigning headerToExtract to the memory area where header is poining to will make it accessible from 
        // outside of the scope of this function.
        header = headerToExtract;
    }
    else
    {
        header = (PSF_HEADER)malloc(sizeof(SF_HEADER));
    }
    

    if ( ( fd = open(filePath, O_RDONLY) ) < 0)
    {
        LOG_ERROR("%s:No such file or directory.", filePath);
        return STATUS_INVALID_PARAMETERS;
    }
    // Fetch header size and magic nr
    lseek(fd, -3, SEEK_END);
    
    if ( read(fd, buffer, 3) )
    {
        // Begin magic number validation
        if ( buffer[2] != '\x30' ) // '0'
            goto fail_magic;

        // Get header size
        *headerSize = *((unsigned short*)buffer);

        // Check version & sections number
        lseek(fd, - (*headerSize), SEEK_END);

        if ( read(fd, buffer,2) )
        {   
            
            // Check version 
            if (  !( (unsigned char)buffer[0] >= 51 && (unsigned char)buffer[0] <= 145 ) )
                    goto fail_version;
                    

            header->version = (unsigned short)buffer[0];

            // Check if valid section number
            if ( !( (unsigned char)buffer[1] >= 8 && (unsigned char)buffer[1] <= 15 ) )
                goto fail_section_number;

            header->noOfSections = (unsigned short)buffer[1];

            header->sections = (PSF_HEADER_SECTION)malloc( header->noOfSections * sizeof( SF_HEADER_SECTION ) );

            // Check section validity (naive check: no boundary checking etc.)
            while( read( fd, buffer, SF_SECTION_HEADER_SIZE ) && cnt != header->noOfSections  )
            {
                // type starts from buffer[13]
                if( !((int)buffer[13] == 31 || (int)buffer[13] == 14
                || (int)buffer[13] == 66 || (int)buffer[13] == 47) )
                    goto fail_section_type; // fix

                header->sections[cnt].sectName = (char*)malloc(13 * sizeof(char));

                strncpy( header->sections[cnt].sectName, buffer, 13 );

                header->sections[cnt].sectType = (unsigned short)buffer[13];

                memcpy( &(header->sections[cnt].sectOffset), buffer + 17, 4 );

                memcpy( &(header->sections[cnt].sectSize), buffer + 21, 4 );

                cnt++;
            }
        }

    }
    else
    {
        if ( verbose )
            LOG_ERROR("Error in reading file. GLE=%x", errno);
        goto fail;
    }

    
    if ( verbose )
    {
        printf("SUCCESS\n version=%d \n nr_sections=%d\n", header->version, header->noOfSections);
        for( int i = 0; i < header->noOfSections; i++)
        {
            printf("section%d: %s %d %d\n", i+1, header->sections[i].sectName, 
            header->sections[i].sectType, header->sections[i].sectSize);
        }
    }


    // If not keepHeader, also free the header structure
    if ( !keepHeader ) freeHeader(&header);
    close(fd);
    free(buffer);
    free(headerSize);

    return STATUS_SUCCESS;

    fail_magic:
        if (verbose)
            puts("ERROR\nwrong magic\n");
        goto fail;

    
    fail_version:
        if (verbose)
            puts("ERROR\nwrong version\n");
        goto fail;

    fail_section_number:
        if (verbose)
            puts("ERROR\nwrong sect_nr\n");
        goto fail;

    fail_section_type:
        if (verbose)
            puts("ERROR\nwrong sect_types\n");
        for( int i = 0; i < cnt; i++ )
            free( header->sections[i].sectName );
        free(header->sections);
        goto fail;


    fail:
        close(fd);
        free(buffer);
        free(headerSize);
        free(header);
        return STATUS_NOT_SF;

}

/**
 * @brief Extracts information about SF files. 
 * @param filePath Path to the desired file
 * @param section Section to extract from
 * @param line Line to extract from
 * @return STATUS variable with a corresponding meaning
 */
STATUS extract( const char* filePath, const char* section, const char* line )
{
    STATUS check = STATUS_INVALID_STATUS;

    PSF_HEADER header = NULL;
    int fd;
    int intSection;
    int intLine;
    int countLines;
    char* fileBuffer;
    int offsetLineStart;
    int offsetLineEnd;
    size_t readBytes;

    if ( NULL == filePath || NULL == section || NULL == line )
    {
        return STATUS_INVALID_PARAMETERS;
    }

    header = (PSF_HEADER)malloc(sizeof( SF_HEADER ));

    // Extract header structure.
    check = parse( filePath, FALSE, TRUE, header);

    if ( check != STATUS_SUCCESS )
    {
        printf("ERROR\ninvalid file");
        return STATUS_INVALID_PARAMETERS;
    }

    if ( ( fd = open( filePath, O_RDONLY ) ) < 0 )
    {
        printf("ERROR\ncannot open file");
        return STATUS_FAILED;
    }

    // Fetch lines / sections
    sscanf( section, "%d", &intSection );
    sscanf(line , "%d", &intLine);

    // Check for validity of section
    // Sections start from 0 but assignment requires them to start from 1
    if ( header->noOfSections + 1 < intSection )
    {
        printf("ERROR\ninvalid section");
        return STATUS_INVALID_PARAMETERS;
    }

    //Start reading line
    lseek( fd, header->sections[ intSection - 1 ].sectOffset, SEEK_SET );

    countLines = 1; readBytes = 0; offsetLineStart = 0; offsetLineEnd = 0;
    fileBuffer = malloc(100000);


    // Assume line is present in the section
    while( ( read( fd, fileBuffer,  512) ) )
    {
        if (  ( readBytes > header->sections[ intSection - 1 ].sectSize ) && ( offsetLineEnd ) )
        {
            break;
        }

        for ( int i = 0; i < 512; i++ )
        {

            if ( countLines == intLine - 1 )
            {
                offsetLineStart = readBytes + i + 1 + header->sections[ intSection - 1 ].sectOffset + 1;
            }

            if ( countLines == intLine )
            {
                offsetLineEnd = readBytes + i + 1 + header->sections[ intSection - 1 ].sectOffset;
            }

            if ( fileBuffer[i] == 0xD && fileBuffer[i+1] == 0xA )
            {
                if ( intLine == 1 )
                {
                    offsetLineStart = header->sections[ intSection - 1].sectOffset;
                    offsetLineEnd = i + header->sections[ intSection - 1].sectOffset; 
                    goto success;
                }
                countLines++;
            }


        }

        readBytes += 512;

    }

    success:

    // Fetch line;

    memset( fileBuffer, '\0', 512 );

    fileBuffer[ offsetLineEnd - offsetLineStart  ] = '\0';

    lseek(fd, offsetLineStart, SEEK_SET);

    printf("SUCCESS\n");

    if ( ( read( fd, fileBuffer,  offsetLineEnd - offsetLineStart) ) < 0  )
    {
        LOG_ERROR(" Reading failed. GLE = %x ", errno);
        _exit(-1);
    }
    
    printf("%s", fileBuffer);
    

    free(fileBuffer);
    freeHeader(&header);
    close(fd);

    return STATUS_SUCCESS;
}


STATUS countNewlines( PSF_HEADER header, char* filePath, int* numberOfDesiredLines )
{
    int fd;
    char* buffer;
    int lineLocalNumber;

    if (  ( fd = open( filePath, O_RDONLY ) ) < 0 )
    {
        return STATUS_INVALID_PARAMETERS;
    }

    for ( int i = 0; i < header->noOfSections; i++ )
    {   
        lineLocalNumber = 0;
        // Get beginning of section
        lseek( fd, header->sections[i].sectOffset, SEEK_SET );
    
        buffer = malloc( header->sections[i].sectSize + 1 );

        read( fd, buffer, header->sections[i].sectSize );
        buffer[ header->sections[i].sectSize ] = '\0';

        for ( int j = 0; j < header->sections[i].sectSize; j++ )
        {
            if ( (char)buffer[j] == 0xd && (char)buffer[j+1] == 0xa  )
                lineLocalNumber++;
        }


        if ( ( lineLocalNumber == 16  &&  ( buffer[strlen(buffer) - 1] == 0xa 
        && buffer[ strlen(buffer) - 2 ] == 0xd ) ) || lineLocalNumber == 15 )
            (*numberOfDesiredLines)++;

        free(buffer);
    }

    close(fd);
    
    return STATUS_SUCCESS;
}


void freeHeader( PSF_HEADER* header )
{
    for ( int i = 0; i < (*header)->noOfSections; i++ )
        free( (*header)->sections[i].sectName );

    free( (*header)->sections );
    free( (*header) );
    header = NULL;
}


