// The MIT License (MIT)
// 
// Copyright (c) 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>


#define MAX_NUM_ARGUMENTS 4

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

/*

  Structure to store field values from boot partition sector

*/
struct BPB_struct{
  
  /*
    There are other fields in the Boot Sector, but these are the fields required by us.
  */
  
  int16_t BPB_BytesPerSec;// offset 11 size 2 | count of bytes per sector
                          // possible values 512, 1024, 2048, 4096
  
  int8_t BPB_SecPerClus;  // offset 13 size 1 | no. of sectors per allocation unit
                          // possible values powers o 2 greater than 1. > 2^0
  
  int16_t BPB_RsvdSecCnt; // offset 14 size 2 | number of reserved sectors in Reserved region 
                          // starting from first sector. Never 0, usually 32 for FAT32
  
  int8_t BPB_NumFATs;     // offset 16 size 1 | count of FAT data structures on volume
                          // value is usually 2, any value greater than equal to 1.

  int32_t BPB_FATSz32;    // offset 36 size 4 | count of sectors occupied by ONE FAT.
                          // only defined for FAT32
};

/*Directory structure mentioned in the FAT32 pdf*/
struct __attribute__((__packed__)) DirectoryEntry
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

/*Copied from the pdf provided in the github*/
int LBAToOffset(int32_t sector, struct BPB_struct* bpb);

/*Converts and prints decimal to hexadecimal*/
void printToHex(int num);

/*Copied from compare method provided in the github*/
int compare(char* IMG_Name, char * input);

int LBAToOffset(int32_t sector, struct BPB_struct* bpb)
{
    if(sector == 0)
      return (bpb->BPB_NumFATs * bpb->BPB_FATSz32 * bpb->BPB_BytesPerSec) +
                    (bpb->BPB_RsvdSecCnt * bpb->BPB_BytesPerSec);
    return ((sector - 2) * bpb->BPB_BytesPerSec) + 
            (bpb->BPB_BytesPerSec * bpb->BPB_RsvdSecCnt) + 
            (bpb->BPB_NumFATs * bpb->BPB_FATSz32 * bpb->BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector, FILE * fptr, struct BPB_struct * bpb)
{
    uint32_t FATAddress = (bpb->BPB_BytesPerSec * bpb->BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(fptr, FATAddress, SEEK_SET);
    fread(&val, 2, 1, fptr);
    return val;
}

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  bool is_open = false;
  bool in_root = false;
  FILE * fptr;
  struct BPB_struct * bpb = calloc(1, sizeof(struct BPB_struct));
  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
    
    /*Implement quit/ exit command*/
    if(strcmp(token[0],"exit") == 0 || strcmp(token[0],"quit") == 0)
    {
      break;
    }
    
    /*Implementing open command*/
    if(strcmp(token[0], "open") == 0)
    {
      if(!is_open)
      {
        fptr = fopen(token[1], "r");
        is_open = true;
        if(!fptr)
        {
          printf("Error: File system image not found\n");
          is_open = false;
        }
        if(fptr)
        {
          fseek(fptr, 11, SEEK_SET);                    // ptr moves to 11.
          fread(&(bpb->BPB_BytesPerSec), 2, 1, fptr );  // reads from 11, ptr moves to 13.
          fread(&(bpb->BPB_SecPerClus), 1, 1, fptr);    // reads from 13, ptr moves to 14.
          fread(&(bpb->BPB_RsvdSecCnt), 2, 1, fptr);    // reads from 14, ptr moves to 16.
          fread(&(bpb->BPB_NumFATs), 1, 1, fptr);       // reads from 16, ptr moves to 17.
          fseek(fptr, 36, SEEK_SET);                    // ptr moves to 36
          fread(&(bpb->BPB_FATSz32), 4, 1, fptr);       // reads from 36, ptr moves to 40.
          // move to root address.
          int address = (bpb->BPB_NumFATs * bpb->BPB_FATSz32 * bpb->BPB_BytesPerSec) +
                    (bpb->BPB_RsvdSecCnt * bpb->BPB_BytesPerSec);
          fseek(fptr, address, SEEK_SET);
          int i;
          // populate the directories.
          in_root = true;
          for(i = 0; i<16; i++)
          {
            fread(&dir[i], 32, 1, fptr);
          }
        }
      }
      else
      {
        printf("Error: File system image already open\n");
      }
    }
    
    /*Implementing close command*/
    else if(strcmp(token[0], "close") == 0)
    {
      if(is_open)
      {
        fclose(fptr);
        is_open = false;
      }
      else
      {
        printf("Error: File system is not open\n");
      }
    }
    else if(!is_open)
    {
      printf("Error: File system image should be opened first\n");
    }
    
    /*Implementing bpb command*/
    else if(is_open && strcmp(token[0], "bpb") == 0)
    {
      printf("\n");
      printf("BPB_BytesPerSec: %d\n", bpb->BPB_BytesPerSec);
      printf("BPB_BytesPerSec: ");
      printToHex(bpb->BPB_BytesPerSec);
      printf("\n");
      printf("BPB_SecPerClus: %d\n", bpb->BPB_SecPerClus);
      printf("BPB_SecPerClus: ");
      printToHex(bpb->BPB_SecPerClus);
      printf("\n");
      printf("BPB_RsvdSecCnt: %d\n", bpb->BPB_RsvdSecCnt);
      printf("BPB_RsvdSecCnt: ");
      printToHex(bpb->BPB_RsvdSecCnt);
      printf("\n");
      printf("BPB_NumFATs: %d\n", bpb->BPB_NumFATs);
      printf("BPB_NumFATs: ");
      printToHex(bpb->BPB_NumFATs);
      printf("\n");
      printf("BPB_FATSz32: %d\n", bpb->BPB_FATSz32);
      printf("BPB_FATSz32: ");
      printToHex(bpb->BPB_FATSz32);
      printf("\n");
    }
    
    /*Implementing Stat command*/
    else if(is_open && strcmp(token[0], "stat") == 0)
    {
      if(!(token[1]))
      {
        printf("Error: File not found\n");
      }
      else
      {
        bool found = false;
        int i;
        for(i = 0; i<16; i++)
        {
          if(dir[i].DIR_Name[0] == (char)0xe5)
            continue;
          char * str = calloc(11, 8);
          char * temp = calloc(11, 8);
          if(dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32)
          {
            memcpy(str,dir[i].DIR_Name,11);
            strcpy(temp, token[1]);
            if(compare(str, temp) == 0)
            {
              found = true;
              printf("File Attribute\t|Size\t\t|Starting Cluster Number\n");
              printf("%d\t\t|%d\t\t|%d\n", dir[i].DIR_Attr, dir[i].DIR_FileSize, 
                                                  dir[i].DIR_FirstClusterLow);
              break;
            }
          }
          free(str);
          free(temp);
        }
        if(!found)
        {
          printf("Error: File not found\n");
        }
      }
    }
    
    /*Implementing get command*/
    else if(strcmp(token[0], "get") == 0)
    {
        char * out_file;
        if(token[2] == NULL)
          out_file = token[1];
        else
          out_file = token[2]; 
        FILE * out_ptr = fopen(out_file,"w");
        int i;
        uint16_t sector;
        uint16_t sizeOfFile;
        bool found = false;
        for(i = 0; i< 16; i++)
        {
          if(dir[i].DIR_Name[0] == (char)0xe5)
            continue;
          char * str = calloc(11, 8);
          char * temp = calloc(11, 8);
          if(dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32)
          {
            memcpy(str,dir[i].DIR_Name,11);
            strcpy(temp, token[1]);
            if(compare(str, temp) == 0)
            {
              found = true;
              sizeOfFile = dir[i].DIR_FileSize;
              // check if its a folder.
              if(sizeOfFile == 0)
              {
                found = false;
                continue;
              }
              sector = dir[i].DIR_FirstClusterLow;
              break;
            }
          }
          free(str);
          free(temp);
        }
        uint32_t remainingSize = sizeOfFile/8;
        while(sector != -1 && remainingSize>0)
        {
          char *content = malloc(bpb->BPB_BytesPerSec);
          fseek(fptr, LBAToOffset(sector, bpb), SEEK_SET);
          if(remainingSize > bpb->BPB_BytesPerSec)
          {
            fread(content, bpb->BPB_BytesPerSec, 1, fptr);
            fwrite(content, bpb->BPB_BytesPerSec, 1, out_ptr);
            remainingSize = remainingSize - bpb->BPB_BytesPerSec;
          }
          else
          {
            fread(content, remainingSize, 1, fptr);
            fwrite(content, remainingSize, 1, out_ptr);
            remainingSize = 0;
          }
          free(content);
          sector = NextLB(sector, fptr, bpb);
        }
        fclose(out_ptr);
        if(!found)
        {
          printf("Error: Unable to find the file '%s'\n", token[1]);
        }
    }
    /*Implemnting ls command*/
    else if(is_open && strcmp(token[0], "ls") == 0)
    {
      int i;
      for(i = 0; i<16; i++)
      {
        if(dir[i].DIR_Name[0] == (char)0xe5)
          continue;
        char * str = calloc(11, 8);
        if(dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32 || dir[i].DIR_Attr == 48)
        {
          memcpy(str,dir[i].DIR_Name,11);
          printf("%s\n", str);
        }
        free(str);
      }
    }
    /*Implementing CD command*/
    else if(is_open && strcmp(token[0], "cd") == 0)
    {
      //check is absolute path
      if(strncmp(token[1],"/",1) == 0)
      {
        fseek(fptr, LBAToOffset(0, bpb), SEEK_SET);
        in_root = true;
        int j;
        for(j=0; j<16; j++)
          fread(&dir[j], 32, 1, fptr);
      }
      
      //continue.
      if(token[1])
      {
        char * new_dir;
        char * copy_token = strdup(token[1]);
        while((new_dir = strsep(&copy_token,"/"))!=NULL)
        {
          if(strcmp(new_dir,"") == 0)
            continue;  
          if(in_root && strcmp(new_dir,"..") == 0)
          {
            printf("Already in root, Can't go to parent directory\n");
            break;
          }
          else 
          {
            int i;
            bool found = false;
            for(i = 0; i< 16; i++)
            {
              if(dir[i].DIR_Name[0] == (char)0xe5)
                continue;
              char * str = calloc(11, 8);
              char * temp = calloc(11, 8);
              if(dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32)
              {

                memcpy(str,dir[i].DIR_Name,11);
                strcpy(temp, new_dir);
                if(strcmp(temp,".") == 0)
                {
                  found = true;
                  break;
                }
                if(strcmp(temp,"..") != 0 && compare(str, temp) == 0 || 
                   strcmp(temp,"..") == 0 && strncmp(str,"..",2) == 0)
                {
                  found = true;
                  uint32_t sizeOfFile = dir[i].DIR_FileSize;
                  if(sizeOfFile != 0)
                  {
                    found = false;
                    continue;
                  }
                  fseek(fptr, LBAToOffset(dir[i].DIR_FirstClusterLow, bpb), SEEK_SET);
                  in_root = false;
                  if(dir[i].DIR_FirstClusterLow == 0)
                  {
                    in_root = true;
                  }
                  int j;
                  for(j=0; j<16; j++)
                    fread(&dir[j], 32, 1, fptr);
                  break;
                }
              }
              free(str);
              free(temp);
            }
            if(!found)
            {
              printf("Error: Unable to find the directory '%s'\n", new_dir);
              break;
            }  
          }
        }
      }
    }
    else if(is_open && strcmp(token[0], "read") == 0)
    {
      int i;
      bool found  = false;
      if(token[1]!= NULL && token[2]!=NULL && token[3]!=NULL)
      {
        for(i = 0; i<16; i++)
        {
          if(dir[i].DIR_Name[0] == (char)0xe5)
            continue;
          char * str = calloc(11, 8);
          char * temp = calloc(11,8);
          if(dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32)
          {
            memcpy(str,dir[i].DIR_Name,11);
            strcpy(temp, token[1]);
            if(compare(str, temp) == 0)
            {
              found = true;
              uint32_t sizeOfFile = dir[i].DIR_FileSize;
              if(sizeOfFile == 0)
              {
                found = false;
                continue;
              }
              fseek(fptr, LBAToOffset(dir[i].DIR_FirstClusterLow, bpb) + atoi(token[2]), SEEK_SET);
              int size = atoi(token[3]);
              int x = 0;
              for(x = 0; x < size; x++)
              {
                uint32_t content;
                fread(&content, 1, 1, fptr);
                printToHex(content);
                printf(" ");
              }
              printf("\n");
              break;
            }
          }
          free(str);
          free(temp);
        }
        if(!found)
        {
          printf("Error: Unable to find the file '%s'\n", token[1]);
        }   
      }
      else
      {
        printf("Please specify other arguments\n");
      }
    }    
    
    // Now print the tokenized input as a debug check    
    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }

    free( working_root );

  }
  return 0;
}


void printToHex(int num)
{
  int ret[30];
  int i = 0;
  if(num == 0)
  {
    printf("0");
  }
  while(num>0)
  {
    ret[i++] = num%16;
    num = num/16;
  }
  i--;
  // printf("0x");
  while(i>=0)
  {
    if(ret[i] < 10)
      printf("%d", ret[i]);
    else
      printf("%c", ret[i] + 55);
    i--;
  }
}


int compare(char* IMG_Name, char * input)
{
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    return 0;
  }
  return -1;
}