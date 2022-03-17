//
// Created by ly on 2022/3/14.
//

#include "mesh_shading_correction.h"


void  generate_mlsc_gain(uint16* img,raw_info rawInfo,mesh_grid_info *meshGridInfo,mesh_gain *meshGain){

    uint16 raw_width = rawInfo.u16ImgWidth;
    uint16  raw_height = rawInfo.u16ImgHeight;
    calc_grid_info(0,raw_width,MLSC_ZONE_COL,meshGridInfo->au16GridXPos);
    meshGridInfo->au16GridXPos[MLSC_ZONE_COL] = raw_width -1;
    calc_grid_info(0,raw_height,MLSC_ZONE_ROW,meshGridInfo->au16GridYPos);
    meshGridInfo->au16GridYPos[MLSC_ZONE_ROW] = raw_height -1;

    mesh_gain mesh_mean;
    uint16 half_mesh_width = (uint16)((meshGridInfo->au16GridXPos[MLSC_ZONE_COL] - meshGridInfo->au16GridXPos[MLSC_ZONE_COL-1])/2);
    uint16 half_mesh_height = (uint16)((meshGridInfo->au16GridYPos[MLSC_ZONE_ROW] - meshGridInfo->au16GridYPos[MLSC_ZONE_ROW-1])/2);
    memset(&mesh_mean, 0, sizeof (mesh_gain));

    uint32 sumR =0,sumGR = 0,sumGB = 0,sumB = 0;
    uint32 cntR = 0,cntGR = 0,cntGB = 0,cntB = 0;
    for(uint8 j = 0;j < MLSC_ZONE_ROW+1;j++){
        for(uint8 i = 0;i< MLSC_ZONE_COL+1;i++){
            int16 start_x = meshGridInfo->au16GridXPos[i] - half_mesh_width;
            int16 end_x = meshGridInfo->au16GridXPos[i] + half_mesh_width;
            int16 start_y = meshGridInfo->au16GridYPos[j] - half_mesh_width;
            int16 end_y = meshGridInfo->au16GridYPos[j] + half_mesh_width;

            if(start_x < 0){
                start_x = 0;
            }
            if(start_y < 0){
                start_y = 0;
            }
            if(end_x > raw_width -1){
                end_x = raw_width -1;
            }
            if(end_y > raw_height -1){
                end_y = raw_height -1;
            }
            for(uint16 y = start_y;y < end_y;y++){
                for(uint16 x = start_x;x < end_x;x++){

                    if( 0 == bayerPattLUT[0][y & 0x1][x & 0x1]){
                        cntR++;
                        sumR = sumR + img[y*raw_width+x];
                     }
                    if( 1 == bayerPattLUT[0][y & 0x1][x & 0x1]){
                        cntGR++;
                        sumGR = sumGR + img[y*raw_width+x];
                     }
                    if( 2 == bayerPattLUT[0][y & 0x1][x & 0x1]){
                        cntGB++;
                        sumGB = sumGB + img[y*raw_width+x];
                    }
                    if( 3 == bayerPattLUT[0][y & 0x1][x & 0x1]){
                        cntB++;
                        sumB = sumB + img[y*raw_width+x];
                    }

                }//end x
            }//end y
            mesh_mean.lsc_gain[j][i][0] = (float)sumR/DIV_0_TO_1(cntR);
            cntR =0;
            sumR = 0;
            mesh_mean.lsc_gain[j][i][1] = (float)sumGR/DIV_0_TO_1(cntGR);
            cntGR =0;
            sumGR = 0;
            mesh_mean.lsc_gain[j][i][2] = (float)sumGB/DIV_0_TO_1(cntGB);
            cntGB =0;
            sumGB = 0;
            mesh_mean.lsc_gain[j][i][3] = (float)sumB/DIV_0_TO_1(cntB);
            cntB =0;
            sumB = 0;

        }//end i
    }//end j
    float  center_R = mesh_mean.lsc_gain[MLSC_ZONE_ROW/2][MLSC_ZONE_COL/2][0];
    float  center_GR = mesh_mean.lsc_gain[MLSC_ZONE_ROW/2][MLSC_ZONE_COL/2][1];
    float  center_GB = mesh_mean.lsc_gain[MLSC_ZONE_ROW/2][MLSC_ZONE_COL/2][2];
    float  center_B = mesh_mean.lsc_gain[MLSC_ZONE_ROW/2][MLSC_ZONE_COL/2][3];
    for(uint8 m = 0;m < MLSC_ZONE_ROW+1;m++){
        for(uint8 n = 0;n< MLSC_ZONE_COL+1;n++) {
            meshGain->lsc_gain[m][n][0] = CLIP_MIN((float)center_R/mesh_mean.lsc_gain[m][n][0],1.0);
            meshGain->lsc_gain[m][n][1] = CLIP_MIN((float)center_GR/mesh_mean.lsc_gain[m][n][1],1.0);
            meshGain->lsc_gain[m][n][2] = CLIP_MIN((float)center_GB/mesh_mean.lsc_gain[m][n][2],1.0);
            meshGain->lsc_gain[m][n][3] = CLIP_MIN((float)center_B/mesh_mean.lsc_gain[m][n][3],1.0);
        }
    }
    print_mesh_gain_to_txt(meshGain);
    return;
}

void  apply_mlsc_gain(uint16* img,raw_info raw_info,mesh_grid_info *XmeshGridInfo,mesh_gain *XmeshGain){

    float lsc_gain;
    uint16 img_height = raw_info.u16ImgHeight;
    uint16 img_width = raw_info.u16ImgWidth;
    uint16 x_index,y_index;
    for(uint16 y = 0;y< img_height; y++){
        for(uint16 x = 0;x < img_width; x++){

            lsc_gain = Bilinear_Inter(x, y, XmeshGridInfo->au16GridXPos, XmeshGridInfo->au16GridYPos,XmeshGain);
            //printf("LINE-%d,lsc_gain:%f \n",__LINE__,lsc_gain);
            img[y*img_width+x] = CLIP_MAX((uint16)(img[y*img_width+x]*lsc_gain),4095);
        }
    }
    return;
}
int main()
{
    /*   int in = 0;
       struct stat sb;
       in = open("lsc_test.raw", O_RDONLY);
       fstat(in, &sb);
       printf("File size is %ld\n", sb.st_size);
   */
    raw_info raw_info;
    raw_info.u16StartX = 0;
    raw_info.u16StartY = 0;
    raw_info.u16ImgHeight = 1080;
    raw_info.u16ImgWidth = 1920;
    raw_info.u16BitDepth = 12;
    raw_info.u8BayerPattern = BPRG;//0 - RGGB
    uint16 blc_value[4] = {240,240,240,240};//RGGB
    mesh_grid_info *meshGridInfo;
    mesh_gain *meshGain;

    meshGridInfo = (mesh_grid_info*)malloc(sizeof(mesh_grid_info));
    if (NULL == meshGridInfo) {
        printf("meshGridInfo malloc fail!\n");
        return 0;
    }
    meshGain = (mesh_gain*)malloc(sizeof(mesh_gain));
    if (NULL == meshGain) {
        printf("meshGain malloc fail!\n");
        return 0;
    }
    memset(meshGridInfo, 0, sizeof (mesh_grid_info));
    memset(meshGain, 0, sizeof (mesh_gain));

    uint16* BayerImg =(uint16*) malloc(1920*1080*sizeof (uint16));
    if(NULL == BayerImg){
        printf("MALLOC FAIL!!!");
    }
    read_BayerImg("D:\\leetcode_project\\lsc_1920x1080_12bits_RGGB_Linear.raw",1080,1920,BayerImg);
    generate_mlsc_gain(BayerImg,raw_info,meshGridInfo,meshGain);
    apply_mlsc_gain(BayerImg,raw_info,meshGridInfo,meshGain);
    //print_raw_to_txt(BayerImg,raw_info);//print for check
    free(BayerImg);
    free(meshGridInfo);
    free(meshGain);
    return 0;

}
