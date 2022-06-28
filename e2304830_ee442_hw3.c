#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Yusuf Mert Kahraman
// EE442
// HW3
char buffer[512];
struct FAT
{
    int entry;    
};
struct fileList
{
    char fileName[248];
    int firstBlock;
    int fileSize;
};
struct Data
{
    char data[512];    
};
int big2LittleEndian(int value)
{
    // It is taken from https://stackoverflow.com/questions/19275955/convert-little-endian-to-big-endian
    return (((value>>24)&0xff) | ((value<<8)&0xff0000) | ((value>>8)&0xff00) | ((value<<24)&0xff000000));
}
void Format(char *diskname)
{
    FILE *fileptr;
    fileptr = fopen(diskname,"w+");
    // If the disk is already in the directory --> just format it. Else, the disk is created.
    if(fileptr == NULL)
    {
        printf("Error opened file\n");
        exit(1);
    }
    struct FAT FAT[1];
    struct fileList fileList[1];
    FAT[0].entry = 0xFFFFFFFF;
    // memset() is used to fill a block of memory with a particular value.
    // memset(void *ptr,int x, size_t n);
    // ptr --> starting address of memory to be filled.
    // x --> value to be filled
    // n --> number of bytes to be filled starting
    memset(&fileList[0].fileName[0],0,sizeof(fileList[0].fileName));
    fileList[0].firstBlock = 0;
    fileList[0].fileSize = 0;
    int i;
    // set the cursor to the beginning of the file
    fseek(fileptr,0,SEEK_SET);
    // FAT[0].entry = 0xFFFFFFFF
    fwrite(FAT,sizeof(FAT),1,fileptr);
    // other entries will be 0x00000000
    FAT[0].entry = 0x00000000;
    for(i = 1; i < 4096; i++)
    {
        fwrite(FAT,sizeof(FAT),1,fileptr);    
    }
    //format the list
    for(i = 1; i <= 128; i++)
    {
        fwrite(fileList,sizeof(fileList), 1,fileptr);
    }
    fclose(fileptr);
}
void Write(char *diskname, char *source_file, char *destination_file)
{
    FILE *fileptr = fopen(diskname,"r+");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not found or opened.\n",diskname);
        exit(1);
    }
    // creating buffers
    struct FAT FAT[4096];
    struct fileList fileList[128];
    struct Data Data[4096];
    FILE *srcptr;
    srcptr = fopen(source_file, "r+");
    // Read them into the buffers
    fread(FAT,sizeof(FAT),1,fileptr);
    fread(fileList,sizeof(fileList),1,fileptr);
    fread(Data,sizeof(Data),1,fileptr);
    // Find the file
    int i = 0;
    int j = 0;
    int readsize = 0;
    int prevIndex = 1;
    int endian = 0;
    int count = 0;
    while(1)
    {
        // The buffer is clearing.
        memset(&buffer[0], 0, sizeof(buffer));
        // Buffer is reading srcptr
        readsize = fread(buffer, sizeof(char), sizeof(buffer),srcptr);
        // Finding the empty space
        for(i = prevIndex; i < 4096; i++)
        {
            if(FAT[i].entry == 0)
            {
                break;
            }
        }
        if(i == 4096)
        {
            printf("No available space\n");
            fclose(fileptr);
            return;
        }
        FAT[i].entry = 0xFFFFFFFF;
        // change the position of the cursor
        fseek(fileptr, i * sizeof(int), SEEK_SET);
        // convert to little endian
        endian = big2LittleEndian(i);
        if(count == 0)
        {
            fseek(fileptr, 4096 * sizeof(struct FAT) + 128 * sizeof(struct fileList) + i * sizeof(struct Data), SEEK_SET);
            fwrite(&buffer, sizeof(char), readsize, fileptr);
            for(j = 0; j < 128; j++)
            {
                if(fileList[j].firstBlock == 0 && fileList[j].fileSize == 0)
                {
                    break;
                }
            }
            strcpy(&fileList[j].fileName[0],destination_file);
            fileList[j].firstBlock = i;
        }
        else
        {
            FAT[prevIndex].entry = endian;
            fseek(fileptr, 4096 * sizeof(struct FAT) + 128 * sizeof(struct fileList) + i * sizeof(struct Data), SEEK_SET);
            fwrite(&buffer, sizeof(char), readsize, fileptr);
        }
        prevIndex = i;
        count = count + 1;
        if(readsize != sizeof(buffer))
        {
            break;
        }
    }
    fileList[j].fileSize = 512 * (count - 1) + readsize;
    fseek(fileptr, 0, SEEK_SET);
    // SILINEBILIR. TEST ET
    fwrite(FAT, sizeof(FAT), 1, fileptr);
    fwrite(fileList, sizeof(fileList), 1, fileptr);
    //
    fclose(fileptr);
    fclose(srcptr);
}
void Read(char *diskname, char *source_file, char *destination_file)
{
    // Buffer that is read from the source_file
    struct FAT FAT[4096];
    struct fileList fileList[128];
    struct Data Data[4096];
    // Pointer to read
    FILE *fileptr = fopen(diskname,"r+");
    if(fileptr == NULL)
    {
        printf("Disk is not found.\n");
        exit(1);
    }
    // Send them to buffers
    fseek(fileptr, 0, SEEK_SET);
    fread(FAT, sizeof(FAT), 1, fileptr);
    fread(fileList,sizeof(fileList),1, fileptr);
    fread(Data, sizeof(Data), 1 ,fileptr);
    int fileLocation = 0;
    for(fileLocation = 0; fileLocation < 128; fileLocation++)
    {
        if(strcmp(fileList[fileLocation].fileName,source_file) == 0)
        {
            // If the file is found.
            break;
        }
    }
    if(fileLocation == 128)
    {
        printf("%s named file does not exist.\n",source_file);
        fclose(fileptr);
        return;
    }
    // Creating a new file to prepare it.
    FILE *newFile = fopen(destination_file,"w+");
    if(newFile == NULL)
    {
        printf("%s named file is not created.\n",destination_file);
    }
    int index = fileList[fileLocation].firstBlock;
    int fileSize = fileList[fileLocation].fileSize;
    // reading until the end of the file.
    memset(&buffer[0], 0, sizeof(buffer));
    while(1)
    {
        memcpy(&buffer, &Data[index], sizeof(Data[index]));
        if(fileSize > 512)
        {
            fwrite(buffer, sizeof(char) * 512, 1, newFile);
            fileSize = fileSize - 512;
        }
        else
        {
            fwrite(buffer, sizeof(char) * fileSize, 1, newFile);
            // Meaning the end of the file
            break;
        }
        if(FAT[index].entry == 0xFFFFFFFF)
        {
            break;    
        }
        index = big2LittleEndian(FAT[index].entry);
        memset(&buffer[0], 0, sizeof(buffer));
    }
    fclose(fileptr);
    fclose(newFile);
}
void List(char *diskname)
{
    FILE *fileptr = fopen(diskname,"r");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not opened.\n",diskname);
        exit(1);
    }
    // Go to the fileList of the diskname
    struct fileList fileList[128];
    fseek(fileptr, 4096 * sizeof(struct FAT), SEEK_SET);
    // Read file list
    fread(fileList,sizeof(fileList),1,fileptr);
    printf("file name\t file size(bytes)\n");
    for(int i = 0; i < 128; i++)
    {
        if(fileList[i].fileSize != 0)
        {
            printf("%s\t%d\n",fileList[i].fileName,fileList[i].fileSize);
        }
    }
    fclose(fileptr);
    
}
void Delete(char *diskname, char *source_file)
{
    FILE *fileptr = fopen(diskname,"r+");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not opened.\n",diskname);
        exit(1);
    }
    struct fileList fileList[128];
    // Reading the file system to check whether the file is exists or not.
    // Adjust the cursor
    fseek(fileptr, 4096 * sizeof(struct FAT), SEEK_SET);
    fread(fileList, sizeof(fileList), 1, fileptr);
    int i = 0;
    for(i = 0; i < 128; i++)
    {
        if(strcmp(fileList[i].fileName,source_file) == 0)
        {
            break;
        }
    }
    if(i == 128)
    {
        printf("The file %s does not exist.\n",source_file);
        fclose(fileptr);
        return;
    }
    struct FAT FAT[4096];
    struct Data Data[4096];
    // Now the file is found and next stage is to delete it.
    // Read FAT and DATA
    fread(Data, sizeof(Data), 1, fileptr);
    fseek(fileptr, 0, SEEK_SET);
    fread(FAT, sizeof(FAT), 1, fileptr);
    // Deleting the things from fileList
    // Deleting fileName
    memset(&fileList[i].fileName[0], 0, sizeof(fileList[i].fileName));
    int temp = 0;
    int index = fileList[i].firstBlock;
    fileList[i].firstBlock = 0;
    int size = fileList[i].fileSize;
    fileList[i].fileSize = 0;
    while(1)
    {
        // Delete data
        memset(&Data[index].data[0], 0, sizeof(Data[index].data));
        // end of the file: if block
        if(FAT[index].entry == 0xFFFFFFFF)
        {
            break;
        }
        temp = big2LittleEndian(FAT[index].entry);
        FAT[index].entry = 0;
        index = temp;
    }
    FAT[index].entry = 0;
    fseek(fileptr, 0, SEEK_SET);
    fwrite(FAT, sizeof(FAT), 1, fileptr);
    fwrite(fileList, sizeof(fileList), 1, fileptr);
    fwrite(Data, sizeof(Data), 1, fileptr);
    fclose(fileptr);
}
void PrintFileList(char *diskname)
{
    // Reading filelist table and check them and print.
    FILE *fileptr;
    fileptr = fopen(diskname,"r");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not opened.\n",diskname);
        exit(1);
    }
    FILE *filelisttxt;
    filelisttxt = fopen("filelist.txt","w+");
    if(filelisttxt == NULL)
    {
        printf("filelist.txt file is not created\n");
        exit(1);
    }
    struct fileList fileList[128];
    fseek(fileptr,4096 * sizeof(struct FAT), SEEK_SET);
    fread(fileList, sizeof(fileList),1 ,fileptr);
    fseek(filelisttxt,0,SEEK_SET);
    fprintf(filelisttxt,"Item\tFile name\tFirst block\tFile size(Bytes)\n");
    for(int i = 0; i < 128; i++)
    {
    	if(fileList[i].fileSize != 0){
    		fprintf(filelisttxt,"%03d\t%s\t%d\t\t%d\n",i,fileList[i].fileName,fileList[i].firstBlock,fileList[i].fileSize);
    	}
    	else{
    		fprintf(filelisttxt,"%03d\t%s\t\t%d\t\t%d\n",i,"NULL",0,fileList[i].fileSize);
    	}
    }
    fclose(fileptr);
    fclose(filelisttxt);
}
void PrintFAT(char *diskname)
{
    FILE *fileptr;
    fileptr = fopen(diskname,"r");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not opened.\n",diskname);
        exit(1);
    }
    FILE *fattxt;
    fattxt = fopen("fat.txt","w+");
    if(fattxt == NULL)
    {
        printf("fat.txt file is not created\n");
        exit(1);
    }
    struct FAT FAT[4096];
    fseek(fileptr,0,SEEK_SET);
    fread(FAT,sizeof(FAT), 1, fileptr);
    fseek(fattxt, 0, SEEK_SET);
    fprintf(fattxt,"Entry\tValue\t\tEntry\tValue\t\tEntry\tValue\t\tEntry\tValue\n");
    for(int i = 0; i < 4096; i = i + 4){
    fprintf(fattxt,"%04d\t%08X\t%04d\t%08X\t%04d\t%08X\t%04d\t%08X\n",i,FAT[i].entry,i+1,FAT[i+1].entry,i+2,FAT[i+2].entry,i+3,FAT[i+3].entry);
    }
    // OPTIONAL: Instead of hexadecimal, the user can see the decimal version of it.
    /*
    for(int i = 0; i < 4096; i = i + 4){
    	fprintf(fattxt,"%04d\t%d\t%04d\t%d\t%04d\t%d\t%04d\t%d\n",i,big2LittleEndian(FAT[i].entry),i+1,big2LittleEndian(FAT[i+1].entry),i+2,big2LittleEndian(FAT[i+2].entry),i+3,big2LittleEndian(FAT[i+3].entry));
    }
    */
    fclose(fileptr);
    fclose(fattxt);
    
    
}
void Defragment(char *diskname)
{
    FILE *fileptr = fopen(diskname,"r+");
    if(fileptr == NULL)
    {
        printf("Error: %s disk is not opened.\n",diskname);
        exit(1);
    }
    // disk exists so defragment it.
    struct FAT FAT[4096];
    struct fileList fileList[128];
    struct Data Data[4096];
    // Set the cursor to the beginning and read the file
    fseek(fileptr,0,SEEK_SET);
    fread(FAT,sizeof(FAT), 1, fileptr);
    fread(fileList,sizeof(fileList), 1, fileptr);
    fread(Data,sizeof(Data), 1, fileptr);
    // New FAT-fileList-Data c
    struct FAT defragmentedFAT[4096];
    struct fileList defragmentedfileList[128];
    struct Data defragmentedData[4096];
    defragmentedFAT[0].entry = 0xFFFFFFFF;
    int elocation = 0;
    int beginningFile = 1;
    int index = 0;
    int FATindex = 1, prevFATindex = 1;
    for(int i = 0; i < 128; i++)
    {
        if( fileList[i].firstBlock !=0 && fileList[i].fileSize != 0)
        {
            // There is a file.
            memcpy(&defragmentedfileList[elocation], &fileList[i],sizeof(struct fileList));
            defragmentedfileList[elocation].firstBlock = FATindex;
            elocation = elocation + 1;
            index = fileList[i].firstBlock;
            while(1){
                memcpy(&defragmentedData[FATindex], &Data[index], sizeof(struct Data));
                if(beginningFile == 0)
                {
                    // End of the file
                    defragmentedFAT[prevFATindex].entry = big2LittleEndian(FATindex);
                }
                else
                {
                    defragmentedFAT[prevFATindex].entry = 0xFFFFFFFF;
                    beginningFile = 0;
                }
                prevFATindex = FATindex;
                if(FAT[index].entry == 0xFFFFFFFF)
                {
                    defragmentedFAT[FATindex].entry = 0xFFFFFFFF;
                    FATindex = FATindex + 1;
                    beginningFile = 1;
                    break;
                }
                FATindex = FATindex + 1;
                index = big2LittleEndian(FAT[index].entry);
            }
        }
    }
    if(elocation == 0)
    {
        // Meaning that the disk is empty.
        return;
    }
    fseek(fileptr,0, SEEK_SET);
    fwrite(defragmentedFAT, sizeof(defragmentedFAT), 1, fileptr);
    fwrite(defragmentedfileList, sizeof(defragmentedfileList), 1,fileptr);
    fwrite(defragmentedData, sizeof(defragmentedData), 1, fileptr);
    fclose(fileptr);
}
int main(int argc, char *argv[])
{
    if(strcmp(argv[2], "-format") == 0)
		Format(argv[1]);
	else if(strcmp(argv[2], "-write") == 0)
		Write(argv[1], argv[3], argv[4]);
	else if(strcmp(argv[2], "-read") == 0)
		Read(argv[1], argv[3], argv[4]);
	else if(strcmp(argv[2], "-list") == 0)
		List(argv[1]);
    else if(strcmp(argv[2],"-delete") == 0)
        Delete(argv[1], argv[3]);
    else if(strcmp(argv[2],"-printfilelist") == 0)
        PrintFileList(argv[1]);
    else if(strcmp(argv[2],"-printfat") == 0)
        PrintFAT(argv[1]);
    else if(strcmp(argv[2],"-defragment") == 0)
        Defragment(argv[1]);
    else
        printf("Unvalid request.\n");
    return 0;
}
