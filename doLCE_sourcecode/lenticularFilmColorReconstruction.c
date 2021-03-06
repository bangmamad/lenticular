//
// doLCE - do Lenticular film Color rEconstruction -
// Copyright (C) 2012 Joakim Reuteler

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// _______________________________________________________________________ 
//


// lenticularFilmColorRecunstruction.c
// main() providing the command line tool 'doLCE'


#include "imageHandling.h"
#include "imageProcessing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main( int argc, char *argv[] ) {
  // processing control
  int status = 0;
  int troubleshoot = 0;
  int relaxRaster = 0;
  int lockRasterSpacing = 0;
  int highRes = 0;
  
  // constants  
  char greetingText[128] = "";
  sprintf( greetingText, "*\n**\n*** doLCE - do Lenticular film Color rEconstruction ***\n**\n*   [%s ~ %s]\n\n", __DATE__, __TIME__);
  char copyrightText[200] = "This program is free software under the terms of the GNU General Public License version 3.\nSee <http://www.gnu.org/licenses/>.\n";
  char helpText[] = "doLCE [-help] [-highRes] [-profileRelThickness (float)] [-profileRelPosY (float)] [-relaxRaster] [-rasterSpacing (float)] [-troubleshoot] 'inputDir' 'inputBaseName' 'startNo' 'endNo' 'outputDir'\n";
  
  char inputImageName[128] = "";
  char inputDirName[128] = "";
  char inputBaseName[128] =  "";
  char startNoStr[16] = "";
  int startNo;
  char endNoStr[16] = "";
  int endNo;
  char outputDirName[128] = "";
  char rgbFrameName[128] = "";
    
  // running variables
  int argNo;
  int channel;
  int i;
  int frameNo;
  char frameNoFormat[8] = "";
  char frameNoStr[16] = "";
  int base; 

  // data
  int width, height;
  short spp, bps;
  int minGl, maxGl;
  glImage_t glScan;
  glScan.img = NULL;
  glScan.memState = 0;
  glHistogram_t glHist;
  glHist.freq = NULL;
  glHist.memState = 0;
  glHistogram_t fourShotGlHists[4];
  for ( i=0; i<4; i++ ) {
    fourShotGlHists[i].freq = NULL;
    fourShotGlHists[i].memState = 0;
  }
  glProfile_t horProfile;
  horProfile.glSum = NULL;
  horProfile.memState = 0;
  glProfile_t horProfileCpy;
  horProfileCpy.glSum = NULL;
  horProfileCpy.memState = 0;
  rgbImage_t rgbImage;
  rgbImage.img = NULL;
  rgbImage.memState = 0;
  rgbImage_t rgbFrame;
  rgbFrame.img = NULL;
  rgbFrame.memState = 0;
  int rasterSpacing = 0;
  int rasterSize;
  int nMinima;
  int threshold;
  int thickness;
  positionList_t rasterPos;
  rasterPos.list = NULL;
  rasterPos.memState = 0;
  float meanPeakSpacing;
  int nPeaks;
  int peakSpacingHist[40];
  int rgbMinVal[3], rgbMaxVal[3];
  float factors[3]; 
  float buf;
  int rgbFrameWidth, rgbFrameHeight;
  point_t p;
  float profileRelPosY = 0.1;
  float profileRelThickness = 0.8;
  int relaxDist;
  int j;

  int rasterDimX, rasterDimY;
  float subPixelPrecisionRasterSpacing = 19.2;  

  int perfAreaWidth, perfAreaHeight;
  glImage_t perfArea[4];
  for ( i=0; i<4; i++ ) {
    perfArea[i].img = NULL;
    perfArea[i].memState = 0;
  }
  int perfNoX, perfNoY;
  int i_offset, j_offset;
  int frameCenterX, frameCenterY;
  float perfRelGlThresholds[4] = { 0.95, 0.95, 0.95, 0.95 };  

  // troubleshoot stuff
  char glHistFileName[128] = ""; //"glHistogram.txt";
  char horProfileLocalMinimaName[128] = ""; //horizontalProfile_localMinima.txt";
  char horProfileName[128] = ""; //horizontalProfile.txt";
  char horProfileSmooName[128] = "";
  char horProfileRegRasterName[128] = ""; //horizontalProfile_regularizedRaster.txt";
  char outputTIFFname[128] = ""; //output.tif";
  char debugTIFFname[128] = ""; //debugImage.tif";
  char fourShotGlHistsName[128] = ""; //fourShotHistograms.txt";
  
  // disclaimer
  printf( "\n" );
  printf( "%s", greetingText );
  printf( "%s", copyrightText );
  printf( "\n\n" );
  

  // parsing command line input
  if ( argc < 6 ) {
    status = -1;
    printf( "%s", helpText );
    printf( "\n" );
  }
  else {
    printf( "\tPARSING input arguments\n" );
    for ( argNo=1; argNo<argc-5 && status==0; argNo++ ) {
      if ( strlen(argv[argNo])==5 && strncmp(argv[argNo],"-help",5)==0 ) {
        printf( "%s", helpText );
      }
      else if ( strlen(argv[argNo])==8 && strncmp(argv[argNo],"-highRes",8)==0 ) {
        highRes = 1;
        printf( "highRes mode is ON\n" );
      }
      else if ( strlen(argv[argNo])==15 && strncmp(argv[argNo],"-profileRelPosY",15)==0 ) {
        //p.x = atoi( argv[++argNo] );
        profileRelPosY = atof( argv[++argNo] );
        if ( profileRelPosY<0.0 || profileRelPosY>1.0 ) {
          status = -1;
          printf( "ERROR: Invalid value passed by option '-profileRelPosY' : 'profileRelPosY=%f<0.0 || profileRelPosY=%f>1.0'\n", profileRelPosY, profileRelPosY ); 
        }
        else {
          printf( "y-position of upper left corner of gray level profile relative to image heigth : %f\n", profileRelPosY );
        }
      }
      else if ( strlen(argv[argNo])==20 && strncmp(argv[argNo],"-profileRelThickness",20)==0 ) {
        profileRelThickness = atof( argv[++argNo] );
        if ( profileRelThickness<0.0 || profileRelThickness>1.0 ) {
          status = -1;
          printf( "ERROR: Invalid value passed by option '-profileRelThickness' : 'profileRelThickness=%f<0.0 || profileRelThickenss=%f>1.0'\n", profileRelThickness, profileRelThickness ); 
        }
        else {
          printf( "thickness for gray level profile relative to image height: %f\n", profileRelThickness );
        }
      }
      else if ( strlen(argv[argNo])==13 && strncmp(argv[argNo],"-troubleshoot",13)==0 ) {
        troubleshoot = 1;
        printf( "troubleshoot mode is ON\n" );
      }
      else if ( strlen(argv[argNo])==14 && strncmp(argv[argNo],"-rasterSpacing",14)==0 ) {
        lockRasterSpacing = 1;
        argNo++;
        rasterSpacing = atoi( argv[argNo] );
        printf( "lockRasterSpacing is ON\n" );
        printf( "using rasterSpacing = %d\n", rasterSpacing );
      }
      else if ( strlen(argv[argNo])==12 && strncmp(argv[argNo],"-relaxRaster",12)==0 ) {
        relaxRaster = 1;
        printf( "relaxRaster mode is ON\n" );
      }
      else {
        status = -1; 
        printf( "ERROR: invalid option '%s', try '-help'\n", argv[argNo] );
      }
    }
    if ( status == 0 ) {
      if ( argc-argNo < 5 ) {
        status = -1;
        printf( "ERROR: Not enough arguments, try '-help'\n" );
      }
      else if (argc-argNo > 5 ) {
        status = -1;
        printf( "ERROR: Too many arguments, try '-help'\n" ); // actually this cannot really happen since invalid arguments will be detected 
      }
      else {
        //printf( "argNo = %d\n", argNo );
        strcpy( inputDirName, argv[argNo] );
        //printf( "inputDirName = %s\n", inputDirName );
        strcpy( inputBaseName, argv[++argNo] );
        //printf( "inputBaseName = %s\n", inputBaseName );
        strcpy( startNoStr, argv[++argNo] );
        //printf( "startNoStr = %s\n", startNoStr );
        startNo = atoi( argv[argNo] );
        //printf( "startNo = %d\n", startNo );
        strcpy( endNoStr, argv[++argNo] );
        //printf( "endNoStr = %s\n", endNoStr );
        endNo = atoi( argv[argNo] );
        //printf( "endNo = %d\n", endNo );
        strcpy(  outputDirName, argv[++argNo] );
        //printf( "outputDirName = %s\n", outputDirName );
        
        printf( "image sequence to be processed : '%s/%s%s.tif' to '%s/%s%s.tif'\n", inputDirName, inputBaseName, startNoStr, inputDirName, inputBaseName, endNoStr );
        printf( "reconstructed rgb frames will be stored as '%s/%s/rgb_%s%s.tif' etc.\n", inputDirName, outputDirName, inputBaseName, startNoStr ); 
        // check if first and last frame exist, check if output directory exists by opening a log file in there, else prompt to get confirmation in order to create that directory !!!!!!     
        printf( "\nlog of call : '" );
        for ( argNo=0; argNo<argc; argNo++ ) {
          printf( "%s ", argv[argNo] );
        }
        printf( "'\n" );
	
        if ( strlen(startNoStr) == strlen(endNoStr) ) {
          if ( strlen(startNoStr) == 1 ) {
            sprintf( frameNoFormat, "%%d" );
            //printf( "frameNoFormat = %s\n", frameNoFormat );
          }
          else {
            sprintf( frameNoFormat, "%%0%dd", (int)strlen(startNoStr) );
            //printf( "frameNoFormat = %s\n", frameNoFormat );
          }    
        }
        else {
          if ( strlen(startNoStr)>1 && startNoStr[0]==0 ){
            status = -1;
            printf( "ERROR: Cannot handle this kind of numbering\n" );
            printf( "startNoStr[0] == 0\n" );
            printf( "startNoStr == %s\n", startNoStr );
          }
          else {
            sprintf( frameNoFormat, "%%d", strlen(startNoStr) );
            //printf( "frameNoFormat = %s\n", frameNoFormat );
          }
        }
      }
      if ( status == 0 ) {
        printf( "\n-------------------loop over frames----------------------\n\n" );
      }
      for ( frameNo=startNo; frameNo<=endNo && status==0; frameNo++ ) {
	sprintf( frameNoStr, frameNoFormat, frameNo );
        //printf( "frameNoStr = %s\n", frameNoStr );  
        sprintf( inputImageName, "%s/%s%s.tif", inputDirName, inputBaseName, frameNoStr );
        printf( "***************************************************************************\n" );
        printf( "*** processing frame %d/%d ('%s')\n", frameNo-startNo+1, endNo-startNo+1, inputImageName );
        sprintf( rgbFrameName, "%s/%s/rgb_%s%s.tif", inputDirName, outputDirName, inputBaseName, frameNoStr );
        //printf( "\t giving : %s\n", rgbFrameName );
        if ( troubleshoot == 1 ) {
          sprintf( glHistFileName, "%s/%s/glHist_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( horProfileLocalMinimaName, "%s/%s/horizontalProfile_localMinima_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( horProfileName, "%s/%s/horizontalProfile_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( horProfileSmooName, "%s/%s/horizontalProfile_smoothed_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( horProfileRegRasterName, "%s/%s/horizontalProfile_regularizedRaster_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( outputTIFFname, "%s/%s/output_%s%s.tif", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( debugTIFFname, "%s/%s/debugImage_%s%s.tif", inputDirName, outputDirName, inputBaseName, frameNoStr );
          sprintf( fourShotGlHistsName, "%s/%s/fourShotHistograms_%s%s.txt", inputDirName, outputDirName, inputBaseName, frameNoStr );
        }	
        
        if ( status == 0 ) {
          printf( "\tANALYZING frame\n" );
          printf( "Checking TIFF...\n" );
          status = check_TIFF( inputImageName, &spp, &bps, &width, &height );
          if ( status == 0 ) {
            printf( "> % 5d samples per pixel \n> % 5d bits per sample\n", spp, bps );  // change into "normal" format: blabla = value ?
            printf( "> % 5d pixels wide\n> % 5d pixels high\n", width, height );
            if ( glScan.memState == 0 ) {
              new_glImage( &glScan, width, height );
              printf( "### allocated memory: 'glScan.memState' = %d\n", glScan.memState );        
            }
            if ( glScan.memState == 1 ) {
              printf( "Loading TIFF...\n" );
              status = read_16bitTIFF_glImage( inputImageName, &glScan );
              if ( status == 0 ) {
                printf( "> OK.\n" );
              }
            }
            else {
              status = -1;
              printf( "ERROR: No memory for loading TIFF available.\n" );
            }
          }
	  else {
	    status = 0;
	    printf( "WARNING: Frame No %d is missing --> KEEPING PREVIOUS FRAME AND WRITING RESULT TO FILE WITH CURRENT FRAME NUMBER! i.e. frame No %d is duplicte of %d\n", frameNo, frameNo, frameNo-1 );
	  }
        }
        if ( status==0 ) {
          printf( "Checking gray level dynamic...\n" );
          status = minMax_glImage( &glScan, &minGl, &maxGl );
          if ( status == 0 ) {
            printf( "> [% 5d,% 5d] interval of gray levels\n", minGl, maxGl );
            if ( glHist.memState == 0 ) {
              new_glHistogram( &glHist, minGl, maxGl ); 
              printf( "### allocated memory: 'glHist.memState' = %d\n", glHist.memState );        
            }
            if ( glHist.memState == 1 ) {
              printf( "> getting gray level histogram\n" );
              status = get_glHistogram( &glScan, &glHist );
              if ( status == 0 ) {
                printf( "> OK.\n" );
                if ( troubleshoot==1 ) {
                  printf( "> writing gray level histogram to disk\n" );
                  status = write_glHistogram( &glHist, glHistFileName );
                }
              }
            }
          }
        }

	if ( status==0 ) {
	  printf( "Looking for film perforation in corners of gl scan...\n" );
	  perfAreaWidth = 0.05*glScan.width;
	  perfAreaHeight = 0.15*glScan.height;
	  for ( i=0; i<4; i++ ) {
	    if ( perfArea[i].memState == 0 ) {
	      status = new_glImage( &perfArea[i], perfAreaWidth, perfAreaHeight );
	    }
	    else {
	      if ( perfArea[i].width!=perfAreaWidth || perfArea[i].height!=perfAreaHeight ) {
		delete_glImage( &perfArea[i] );
		if ( perfArea[i].memState == 0 ) {
		  status = new_glImage( &perfArea[i], perfAreaWidth, perfAreaHeight );
		}
		else {
		  status = -1;
		  printf( "ERROR: Failed to resize gl images for inspection of gl scan corner to find perforations.\n" );
		}
	      }
	    }
	    if ( perfArea[i].memState != 1 ) {
	      status = -1;
	      printf( "ERROR: Cannot serach for perforation : 'perfAreas.memState != 1'\n" );
	    }
	  }

	  if ( status == 0 ) {
	    for ( perfNoY=0; perfNoY<2; perfNoY++ ) {
	      for ( perfNoX=0; perfNoX<2; perfNoX++ ) {
            //perfX[perfNoX+2*perfNoY] = perfY[perfNoX+2*perfNoY] = 0;
            i_offset = perfNoX*(glScan.width - perfAreaWidth);
            j_offset = perfNoY*(glScan.height - perfAreaHeight);
            for ( j=0; j<perfAreaHeight; j++ ) {
              for ( i=0; i<perfAreaWidth; i++ ) {
                if ( glScan.img[j+j_offset][i+i_offset] >= (int)(maxGl*perfRelGlThresholds[perfNoX+2*perfNoY]+0.5) ) {
                  perfArea[perfNoX+2*perfNoY].img[j][i] = 1;
		      /*		      
		      if ( i > perfX[perfNoX+2*perfNoY] ) {
			perfX[perfNoX+2*perfNoY] = i;
		      }
		      if ( j > perfY[perfNoX+2*perfNoY] ) {
			perfY[perfNoX+2*perfNoY] = j;
		      }
		      */
                }
                else {
                  perfArea[perfNoX+2*perfNoY].img[j][i] = 0;
                }
              }
            }
		// need to process perfArea.img in order to get clean perforation hole geometry, e.g. fit a prescibed shape(how to do this?), or apply morphological closing, extraction of largest connected component, use a regio growth method if possible in conjunction with a rather strict threshold, or crosscorrelation with a hole template once the boundaries of the size of the perforation are extarcted from the gl thresholded set
		// then find a point from which frame position correction can be computed
	      }
	    }
	    // average the proposed frame centers and store this for position correction (true center of frame)
	  }
	}
      
        if ( status == 0 ) {
          printf( "\n" );
          printf( "\tPROCESSING frame\n" );
          printf( "Shifting gray levels...\n" );
          status = addGl_glImage( &glScan, -1*minGl );
          if ( status == 0 ) { 
            printf( "> OK, checking new gray level dynamic...\n" );
            status = minMax_glImage( &glScan, &minGl, &maxGl );
            if ( status == 0 ) {
              printf( "> [% 5d,% 5d] interval of gray levels\n", minGl, maxGl );
            }
          }
          if ( status==0 && troubleshoot==1 ) {
            for ( i=0; i<4 && status==0; i++ ) {    
              if ( fourShotGlHists[i].memState == 0 ) {
                status = new_glHistogram( &(fourShotGlHists[i]), minGl, maxGl ); 
              }
            }
            if ( status == 0 ) {
              for ( i=0; i<4; i++ ) {
                printf( "### allocated memory: fourShotGlHists[%d].memState = %d\n", i, fourShotGlHists[i].memState );
                if ( fourShotGlHists[i].memState != 1 ) {
                  status = -1;
                }
              }
              if ( status == 0 ) {
                printf( "Getting four shot histograms...\n" );
                status = get_glHistograms_fourShot( &glScan, fourShotGlHists );
              }
              if ( status == 0 ) {
                status = write_glHistograms_fourShot( fourShotGlHists, fourShotGlHistsName );
                if ( status == 0 ) {
                  printf( "> Wrote four shot gl histograms to '%s'\n", fourShotGlHistsName );
                }
              }
            }
          }
        }
        
        if ( status == 0 ) { 
          printf( "\n" );         
          printf( "\tLOCALIZING RASTER...\n" );
          printf( "Preparing for getting gray level profile...\n" );
          printf( "Setting absolute values for y-position and thickness of profile...\n" );
          p.x = 0;
          p.y = (int)( profileRelPosY * (glScan.height) + 0.5 );
          thickness = (int)( profileRelThickness * (glScan.height) + 0.5 ); 
          printf( "> upper left corner at (x,y) = (%d,%d)\n", p.x, p.y );
          printf( "> thickness = %d\n", thickness );
          if ( horProfile.memState == 0 ) {
            status = new_glProfile( &horProfile, glScan.width-p.x );
            if ( status == 0 ) {
              printf( "### allocated memory: horProfile.memState = %d\n", horProfile.memState );
              status = new_glProfile( &horProfileCpy, glScan.width-p.x );
              if ( status==0 && relaxRaster==1 ) {
                printf( "### allocated memory: horProfileCpy.memState = %d\n", horProfileCpy.memState );
              }
            }
          }
          else if ( horProfile.memState == 1 ) {
            if ( horProfile.length != glScan.width-p.x ) {
              delete_glProfile( &horProfile );
              delete_glProfile( &horProfileCpy );
              if ( horProfile.memState == 0 ) {
                status = new_glProfile( &horProfile, glScan.width-p.x );
                if ( status==0 && relaxRaster==1 ) {
                  status = new_glProfile( &horProfileCpy, glScan.width-p.x );
                }
              }
              else {
                status = -1;
                printf( "ERROR: Cannot resize memory to suitable size for 'horProfile'.\n" );
              }
            }
          }
          if ( status==0 && horProfile.memState==1 ) {
            printf( "Getting gl profile..." );
            status = get_glProfile( &glScan, &horProfile, p, 0, thickness );
            if ( status == 0 ) {
              printf( "> OK.\n" );
              if ( relaxRaster == 1 ) {
                status = copy_glProfile( &horProfile, &horProfileCpy ); // maybe better to use the smoothed copy?
                if ( status == 0 ) {
                  printf( "> Copied glProfile for later use.\n" );
                }
              }
              if ( troubleshoot==1 ) {
                printf( ">>> storing gl profile to '%s'\n", horProfileName );
                status = write_glProfile( &horProfile, horProfileName ); 
              }
            }
          }
          if ( status == 0 ) {
            printf( "Analyzing gray level profile...\n" );
            printf( "> smoothing the gl profile\n" );
            status = smooth_glProfile( &horProfile );
            if ( status == 0 ) {
              printf( "> OK.\n" );
              if ( troubleshoot == 1 ) {
                printf( ">>> storing smoothed gl profile to '%s'\n", horProfileSmooName );
                write_glProfile( &horProfile, horProfileSmooName ); 
              }
	      /*
              printf( "> computing local minima of the profile\n" );
              status = extract_localMinima_glProfile( &horProfile );
	      */
	      printf( "> extracting wells...\n" );
	      status = extract_wells_glProfile( &horProfile );
              if ( status == 0 ) {
                printf( "> OK.\n" );
                if ( troubleshoot == 1 ) {
                  printf( ">>> storing well gray levels to '%s'\n", horProfileLocalMinimaName );
                  write_glProfile( &horProfile, horProfileLocalMinimaName ); 
                }
		//printf( "-------------- interrupting for devel and debugging ------------------------------------------------------------\n" );
		//status = -1;
                if ( lockRasterSpacing == 0 ) {
                  printf( "Determining the raster spacing...\n" );
                  printf( "> counting number of minima\n" );
                  status = count_peaks_glProfile( &horProfile, &nMinima );
                  if ( status == 0 ) {
                    printf( "> nMinima = %d\n", nMinima );                  
                    printf( "> computing histogram of distances between minima\n" );
                    status = get_peakSpacingHistogram( &horProfile, peakSpacingHist, &nPeaks );
                    printf( "> dist\tcount\n" );
                    if ( status == 0 ) {
                      for ( i=0; i<40; i++ ) {
                        if ( peakSpacingHist[i] > 0 ) {
                          printf( "> %d\t%d\n", i, peakSpacingHist[i] ); 
                        }
                      }
                      status = get_rasterSpacing( peakSpacingHist, &rasterSpacing );
                      if ( status == 0 ) {
                        printf( "> rasterSpacing = %d\n", rasterSpacing ); 
                      }
                    }
                  }
                }
		if ( status == 0 ) {
		  printf( "Checking deviation of determined raster spacing to *currently* hard coded sub pixel precision raster spacing\n");
		  if ( (rasterSpacing-(int)(subPixelPrecisionRasterSpacing+0.5)) > 1 ) {
		    rasterSpacing = (int)(subPixelPrecisionRasterSpacing+0.5); // skipped retaining of tendency (the '+1')
		    printf( "> determined raster spacing too large, have set it to %d\n", rasterSpacing ); 
		  }
		  else if ( (rasterSpacing-(int)(subPixelPrecisionRasterSpacing+0.5)) < 1 ) {
		    rasterSpacing = (int)(subPixelPrecisionRasterSpacing+0.5); // skipped tendency keeping
		    printf( "> determined raster spacing too small, have set it to %d\n", rasterSpacing ); 
		  }
		}
                if ( status ==0 ) {
                  printf( "Regularizing peaks...\n" );
                  //  rasterSize = nPeaks;
                  status = regularize_peakRaster( &horProfile, rasterSpacing, &rasterSize );
                  // skip and continue with color reconstruction if no peaks were detected using the previous raster point list
                  if ( status==0 ) {
                    if ( rasterSize>0 ) {
                    printf( "> OK.\n" );
                    if ( troubleshoot == 1 ) {
                      printf( "> storing profile containing regularized raster to '%s'\n", horProfileRegRasterName );
                      status = write_glProfile( &horProfile, horProfileRegRasterName ); 
                    }
                    if ( status==0 && relaxRaster==1 ) {
                      printf( "Relaxing regularized raster positions into local minima...\n" );
                      status = relax_regPeakRaster( &horProfileCpy, &horProfile );
                    }
                    if ( status == 0 ) {
                      if ( rasterPos.memState == 0 ) {
                        status = new_positionList( &rasterPos, rasterSize );
                        if ( status == 0 ) {
                          printf( "### Allocated memory: 'rasterPos.memState' = %d\n", rasterPos.memState ); 
                        }
                      }
                      if ( rasterPos.memState == 1 ) {
                        if ( rasterPos.length != rasterSize ) {
                          delete_positionList( &rasterPos );
                          if ( rasterPos.memState == 0 ) {
                            status = new_positionList( &rasterPos, rasterSize );
                            if ( status == 0 ) {
                              printf( "### resized memory: 'rasterPos.memState' = %d\n", rasterPos.memState ); 
                            }
                          }
                          else {
                            status = -1;
                            printf( "ERROR: Cannot resize memory for 'rasterPos' point list.\n" );
                          }
                        }
                        if ( status == 0 ) {
                          printf( "> transfering raster positions from profile into list\n" );  
                          status = get_peakPositions( &horProfile, &rasterPos );
                          if ( status == 0 ) {
                            printf( "> OK.\n" );
			    printf( "Correcting odd raster positions...\n" );
			    status = correct_oddPeakPositions( &rasterPos );
			    if ( status == 0 ) {
			      printf( "> OK.\n" );
			    }
                          }
                        }
                      }
                    }
                  }
                  else {
                    printf( "WARNING: 'rasterSize == %d' --> using raster position list from previous frame!\n", rasterSize );
                    rasterSize = rasterPos.length;
                  }
                }
                }
              }
            }
          }
        }
	// }
                
        if ( status == 0 ) {
          printf( "\n" );
          printf( "\tRECONSTRUCTING COLORS...\n" );
          printf( "Computing image dimensions for color reconstructed frame...\n" );
          if ( highRes == 1 ) {
            rgbFrameWidth = 2*(rasterSize-1)-1;
            rgbFrameHeight = ((rgbFrameWidth)*(glScan.height))/glScan.width;
          }
          else {
            rgbFrameWidth = rasterSize-1;
            rgbFrameHeight = (rasterSize*glScan.height/glScan.width)+1;
          }
          printf( "rgbFrame.width = %d, rgbFrame.height = %d\n", rgbFrameWidth, rgbFrameHeight );
          if ( rgbFrame.memState == 0 ) {
            status = new_rgbImage( &rgbFrame, rgbFrameWidth, rgbFrameHeight );    
            if ( status == 0 ) {
              printf( "### Allocated memory: 'rgbFrame.memState' = %d\n", rgbFrame.memState );
            }
          }
          else if ( rgbFrame.memState == 1 ) {
            if ( rgbFrame.width!=rgbFrameWidth || rgbFrame.height!=rgbFrameHeight ) {
              printf( "WARNING: The current raster dimension differs from the one detected in the previous frame. This means the reconstructed rgb frames will have inconsistent dimensions.\n" );
              delete_rgbImage( &rgbFrame );
              if ( rgbFrame.memState == 0 ) {
                status = new_rgbImage( &rgbFrame, rgbFrameWidth, rgbFrameHeight ); 
                if ( status == 0 ) {
                  printf( "### Allocated memory: 'rgbFrame.memState' = %d\n", rgbFrame.memState );
                }
              }
              else {
                status = -1;
                printf( "ERROR: Cannot free rgbFrame for resizing it.\n" );
              }
            }
          }
          else {
            status = -1;
            printf( "ERROR: Unknown memory state: 'rgbFrame.memState' = %d\n", rgbFrame.memState );
          }
          if ( status == 0 ) {
            printf( "Reconstructing the colors from the gray level scan of the frame and the computed raster for the lenticular lenses...\n" );
            if ( highRes == 1 ) {
              status = reconstructInterpolate_colorFrame( &glScan, &rasterPos, &rgbFrame );
            }
            else {
              status = reconstruct_colorFrame( &glScan, &rasterPos, &rgbFrame );
            }
          }
          if ( status == 0  ) {
            printf( "Compressing color range of reconstructed color frame...\n" );
            status = get_range_rgbImage( &rgbFrame, rgbMinVal, rgbMaxVal );
            printf( "rgb range after reconstruction:\nchannel\tmin\tmax -> factor\n" );
            for( channel=0; channel<3; channel++ ) {
              //factors[channel] = 255.0/(rgbMaxVal[channel]);//-rgbMinVal[channel]); // for proper white balance this has to be made in a different manner!!!!
              factors[channel] = 255.0/(rgbMaxVal[channel]-rgbMinVal[channel]); // for proper white balance this has to be made in a different manner!!!!
              printf( "%d\t%d\t%d -> factor %f\n", channel, rgbMinVal[channel], rgbMaxVal[channel], factors[channel] );
            }
            status = subtractVal_rgbImage( &rgbFrame, rgbMinVal );
            /*buf = factors[0];
            if ( buf > factors[1] ) {
              buf = factors[1];
            }
            if ( buf > factors[2] ) {
              buf = factors[2];
            }
            factors[0] = factors[1] = factors[2] = buf;*/
            status = multiplyVal_rgbImage( &rgbFrame, factors );
            status = get_range_rgbImage( &rgbFrame, rgbMinVal, rgbMaxVal );
            printf( "rgb range after compressing to 8bit:\nchannel\tmin\tmax\n" );
            for( channel=0; channel<3; channel++ ) {
              printf( "%d\t%d\t%d\n", channel, rgbMinVal[channel], rgbMaxVal[channel] );
            }
            if ( status == 0 ) {
              if ( (status = write_3x8bitTIFF_rgbImage( &rgbFrame, rgbFrameName )) == 0 ) {
                printf( "> Stored rgb frame to '%s'.\n", rgbFrameName );
              }
            }
          }
          
          if ( troubleshoot == 1 ) {
            printf( "Creating raster localization debug image...\n" ); // conceptionally it is mor clean to do this after the localization procedure happened (as it was done before implementing the processing of a whole frame series)
            if ( rgbImage.memState == 0 ) {
              status = new_rgbImage( &rgbImage, glScan.width, glScan.height );
              if ( status == 0 ) {
                printf( "### allocated memory: horProfile.memState = %d\n", horProfile.memState );
              }
            }
            if ( rgbImage.memState == 1 ) {
              if ( rgbImage.width!=glScan.width || rgbImage.height!=glScan.height ) {
                delete_rgbImage( &rgbImage );
                if ( rgbImage.memState == 0 ) {
                  status = new_rgbImage( &rgbImage, glScan.width, glScan.height );
                  if ( status == 0 ) {
                    printf( "### resized memory: rgbImage.memState = %d\n", rgbImage.memState );
                  }
                  else {
                    status = -1;
                    printf( "ERROR: Could not resize memory for rgbImage.\n" );
                  }
                }
              }
              if ( status == 0 ) {
                printf( "> Compressing gray level dynamic of glScan: [% 5d,% 5d]*%f = [0,255].\n", minGl, maxGl, 255.0/maxGl ); 
                status = multiplyGl_glImage( &glScan, (255.0/maxGl) );
                if ( status == 0 ) {
                  status = minMax_glImage( &glScan, &minGl, &maxGl );
                  if ( status == 0 ) {
                    printf( "> gl range now [% 5d,% 5d]\n", minGl, maxGl );
                  }
                }
              }
              if ( status==0 ) {
                status = setValue_rgbImage( &rgbImage, 0 );
                if ( status == 0 ) {      
                  for ( channel=0; channel<3 && status==0; channel++ ) {
                    status = add_glImage_to_rgbImage( &glScan, &rgbImage, channel );
                  }
                }
              }
              if ( status==0 ) {
                status = draw_raster_rgbImage( &rgbImage, &horProfile, &rasterPos );
                if ( status == 0 ) {
		  printf( "Marking Perforation holes...\n" );
		  for ( perfNoY=0; perfNoY<2; perfNoY++ ) {
		    for ( perfNoX=0; perfNoX<2; perfNoX++ ) {
		      i_offset = perfNoX*(glScan.width - perfAreaWidth);
		      j_offset = perfNoY*(glScan.height - perfAreaHeight);
		      for ( j=0; j<perfAreaHeight; j++ ) {
			for ( i=0; i<perfAreaWidth; i++ ) {
			  if ( perfArea[perfNoX+2*perfNoY].img[j][i] > 0 ) {
			    rgbImage.img[0][j_offset+j][i_offset+i] = 220;
			    rgbImage.img[1][j_offset+j][i_offset+i] = 220;
			    rgbImage.img[2][j_offset+j][i_offset+i] = 0;
			  }
			}
		      }
		    }
		  }
                  if ( (status = write_3x8bitTIFF_rgbImage( &rgbImage, debugTIFFname )) == 0 ) {
                    printf( "> Stored rgb debug image to '%s'.\n", debugTIFFname );
                  }
                }
              }
            }
            else {
              status = -1;
              printf( "ERROR: No memory for debug image\n" );
            }
          }
          
        }
        printf( "*** done processing frame No %d\n", frameNo );
        printf( "***************************************************************************\n\n" );
      }
      if ( status == 0 ) {
        printf( "\n---------------------end loop over frames------------------------\n" );
      }
    }
  }
    
 

  if ( glScan.memState != 0 ) {
    printf( "\n\tCLEANING up heap\n" );
    delete_glImage( &glScan );
    printf( "> freed memory: 'glScan.memState' = %d\n", glScan.memState );
  }
  if ( rgbFrame.memState != 0 ) {
    delete_rgbImage( &rgbFrame );
    printf( "> freed memory: 'glScan.memState' = %d\n", rgbFrame.memState );
  }
  if ( rgbImage.memState != 0 ) {
    delete_rgbImage( &rgbImage );
    printf( "> freed memory: 'glScan.memState' = %d\n", rgbImage.memState );
  }
  if ( glHist.memState != 0 ) {
    delete_glHistogram( &glHist );
    printf( "> freed memory: 'glHist.memState' = %d\n", glHist.memState );
  }
  for ( i=0; i<4; i++ ) {
    if ( fourShotGlHists[i].memState != 0 ) {
      delete_glHistogram( &(fourShotGlHists[i]) );
      printf( "> freed memory: 'fourShotGlHists[%d].memState' = %d\n", i, fourShotGlHists[i].memState );
    }
  }
  if ( horProfile.memState != 0 ) {
    delete_glProfile( &horProfile );
    printf( "> freed memory: 'horProfile.memState = %d'\n", horProfile.memState );
  }
  if ( horProfileCpy.memState != 0 ) {
    delete_glProfile( &horProfileCpy );
    printf( "> freed memory: 'horProfileCpy.memState = %d'\n", horProfileCpy.memState );
  }
  if ( rasterPos.memState != 0 ) {
    delete_positionList( &rasterPos );
    printf( "> freed memory: rasterPos.memState = %d\n", rasterPos.memState );
  }
  for ( i=0; i<4; i++ ) {
    if ( perfArea[i].memState != 0 ) {
      delete_glImage( &perfArea[i] );
      printf( "> freed memory: 'perfArea[%d].memState' = %d\n", i, perfArea[i].memState );
    }
  }
  printf( "\n" );
  
  
  printf( "Exit: status = %d\n", status );
  printf( "\n" );
  return status;
}

