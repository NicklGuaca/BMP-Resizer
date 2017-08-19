#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bmp.h"

int main(int argc, char *argv[])
{
    // ensure proper usage
    if (argc != 4)
    {
        fprintf(stderr, "Usage: ./resize scale infile outfile\n");
        return 1;
    }

    // remember filenames
    float f = atof(argv[1]);
    char *infile = argv[2];
    char *outfile = argv[3];

    // open input file 
    FILE *inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", infile);
        return 2;
    }

    // open output file
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 3;
    }

    // read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

    // ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 || 
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 4;
    }
    
    //setup new file's header with new properties
    int oldPadding =  (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    bi.biWidth = bi.biWidth * f;
    bi.biHeight = bi.biHeight * f;
    int padding =  (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    bi.biSizeImage = ((sizeof(RGBTRIPLE)*bi.biWidth)+padding) *abs(bi.biHeight);
    bf.bfSize = bi.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);
    // write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);
    
    //write to outfile a scaled down bmp
    if (f< 1 && (fmod(f,1)!=0))
    {
        //variables to track when to write pixel
        float modulo = fmod(f,1);
        modulo = round(1/modulo);
        long offset = bi.biWidth*modulo* sizeof(RGBTRIPLE);
        int counterY=0;
        int counterX=0;


        for (int i = 0, biHeight = abs(bi.biHeight); i < biHeight*modulo; i++)
        {
            if (counterY<1)
            {
                for (int j = 0; j < bi.biWidth*modulo; j++)
                {
                    if (counterX==0)
                    {
                        RGBTRIPLE triple;
                        fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
                        fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
                        counterX++;
                    }
                    else if (counterX==modulo-1)
                    {
                        counterX=0;
                        fseek(inptr, sizeof(RGBTRIPLE), SEEK_CUR);
                    }
                    else{
                        counterX++;
                        fseek(inptr, sizeof(RGBTRIPLE), SEEK_CUR);
                    }
                }
                
                fseek(inptr, oldPadding, SEEK_CUR);
                for (int k = 0; k < padding; k++)
                {
                    fputc(0x00, outptr);
                }
                counterY++;
            }
            //skip line
            else
            {
                 if (counterY==(modulo-1))
                 {
                     counterY--;
                 }
                 else{
                     counterY++;
                 }
                 fseek(inptr, offset+oldPadding, SEEK_CUR);
            }
        }
    }
    
    //write to outfile a scaled up bmp
    else if (f >=1)
    {
        //baseInt is the amount of times necessary to repeat horizontally
        int baseInt = f-(fmod(f,1));
        //if f is 1.5, modulo would be .5
        float modulo = f-baseInt;
        //used to count when another pixel to print is necessary
        float moduloCounterX=0;
        long offset = bi.biWidth/f * sizeof(RGBTRIPLE);
        //varialbes to be used for vertical resizing
        int newLine = 1;
        int copyLine = 0;
        float moduloCounterY = 0;

        for (int i = 0, biHeight = abs(bi.biHeight); i < biHeight; i++)
        {
            //handles vertical resizing. Repeats horizontal line just printed if necessary,
            //to get another row and grow image vertically
            //otherwise moves cursor to fresh line in old file
            if (moduloCounterY >=1){
                fseek(inptr, -offset, SEEK_CUR);
                moduloCounterY--;
            }
            else if (copyLine >=1){ 
                fseek(inptr, -offset, SEEK_CUR);
                copyLine--;
            }
            else if (newLine==1 && i!=0){
                fseek(inptr, oldPadding, SEEK_CUR);
                copyLine = baseInt-1;
                newLine--;
            }
            else if(i==0){
                newLine--;
                copyLine = baseInt-1;
            }

            //handles resizing horizontally
            for (int j = 0; j < bi.biWidth/f; j++){      
                RGBTRIPLE triple;
                fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
                for (int iii = 0; iii < baseInt; iii++)
                {
                    fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
                }       
                
                //tracks when to print an extra pixel
                moduloCounterX += modulo;
                if (moduloCounterX>=1){
                    moduloCounterX --;
                    fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
                }
            }
            
            //after repeated, vertical line is printed, set up for a fresh new line
            if (copyLine==0 && newLine==0){
                moduloCounterY+=modulo;
                newLine=1;
            }
            
            //add padding
            for (int k = 0; k < padding; k++){
                fputc(0x00, outptr);
            }
        }
    }
    
    // close infile
    fclose(inptr);

    // close outfile
    fclose(outptr);

    // success
    return 0;
}

