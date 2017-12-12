
//FIXME hack, older jpeg-6b could accept rgba input buffer, now it has to be rgb
static void convert_rgba_to_rgb (byte *buffer, int width, int height)
{
	byte *src;
	byte *dst;
	int totalSize;


	totalSize = width * height * 4;
	src = buffer;
	dst = buffer;
	while (src < (buffer + totalSize)) {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		// skip alpha src[3]

		src += 4;
		dst += 3;
	}
}

// swap rgb to bgr
static void swap_bgr (byte *buffer, int width, int height, qboolean hasAlpha)
{
	int temp;
	int i;
	int c;
	int psize;

	psize = 3 + (hasAlpha ? 1 : 0);

	c = width * height * psize;
	for (i = 0;  i < c;  i += psize) {
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}
}

const void *RB_TakeVideoFrameCmd (const void *data, shotData_t *shotData)
{
	const videoFrameCommand_t	*cmd;
	int												frameSize;
	int												i;
	char finalName[MAX_QPATH];
	qboolean fetchBufferHasAlpha = qfalse;
	qboolean fetchBufferNeedsBGRswap = qfalse;
	int glMode = GL_RGB;
	char *sbuf;
	//__m64 *outAlloc;
	__m64 *outAlign = NULL;
	byte *fetchBuffer;
	int blurFrames;
	int blurOverlap;
	qboolean useBlur;
	int frameRateDivider;

	cmd = (const videoFrameCommand_t *)data;

	R_MME_CheckCvars(qfalse, shotData);
	useBlur = qfalse;
	blurFrames = ri.Cvar_VariableIntegerValue("mme_blurFrames");
	if (blurFrames == 0  ||  blurFrames == 1) {
		useBlur = qfalse;
	} else {
		useBlur = qtrue;
	}
	blurOverlap = ri.Cvar_VariableIntegerValue("mme_blurOverlap");
	if (blurOverlap > 0) {
		useBlur = qtrue;
	}
	frameRateDivider = ri.Cvar_VariableIntegerValue("cl_aviFrameRateDivider");
	if (frameRateDivider < 1) {
		frameRateDivider = 1;
	}

	shotData->pixelCount = cmd->width * cmd->height;
	//outAlloc = ri.Hunk_AllocateTempMemory( shotData->pixelCount * 4 + 8);
	//FIXME
	//outAlign = (__m64 *)((((int)(outAlloc))+7) & ~7);

	if (cmd->png) {
		fetchBufferHasAlpha = qtrue;
		fetchBufferNeedsBGRswap = qtrue;
		glMode = GL_RGBA;
		fetchBuffer = cmd->captureBuffer;
	} else {  //  not png
		sbuf = finalName;
		ri.Cvar_VariableStringBuffer("cl_aviFetchMode", sbuf, MAX_QPATH);
		if (!Q_stricmp("gl_rgba", sbuf)) {
			fetchBufferHasAlpha = qtrue;
			fetchBufferNeedsBGRswap = qtrue;
			glMode = GL_RGBA;
		} else if (!Q_stricmp("gl_rgb", sbuf)) {
			fetchBufferHasAlpha = qfalse;
			fetchBufferNeedsBGRswap = qtrue;
			glMode = GL_RGB;
		} else if (!Q_stricmp("gl_bgr", sbuf)) {
			fetchBufferHasAlpha = qfalse;
			fetchBufferNeedsBGRswap = qfalse;
			glMode = GL_BGR;
		} else if (!Q_stricmp("gl_bgra", sbuf)) {
			fetchBufferHasAlpha = qtrue;
			fetchBufferNeedsBGRswap = qfalse;
			glMode = GL_BGRA;
		} else {
			ri.Printf(PRINT_ALL, "unknown glmode using GL_RGB\n");
			fetchBufferHasAlpha = qfalse;
			fetchBufferNeedsBGRswap = qtrue;
			glMode = GL_RGB;
		}

		if (cmd->jpg  ||  (cmd->avi  &&  cmd->motionJpeg)) {
			//FIXME jpg check not needed anymore
			fetchBuffer = cmd->captureBuffer;
		} else {
			fetchBuffer = cmd->encodeBuffer;
		}
	}

	if (useBlur) {
		glMode = GL_RGBA;
		fetchBufferHasAlpha = qtrue;
		fetchBufferNeedsBGRswap = qtrue;
	}

	if (cmd->jpg  ||  (cmd->avi  &&  cmd->motionJpeg)) {
		// no need to use data as bgr
		fetchBufferNeedsBGRswap = !fetchBufferNeedsBGRswap;
	}

	if (!useBlur) {
		//ri.Printf(PRINT_ALL, "no blur pic count: %d\n", cmd->picCount + 1);
		if ((cmd->picCount + 1) % frameRateDivider != 0) {
			//ri.Printf(PRINT_ALL, "    skipping %d\n", cmd->picCount + 1);
			goto dontwrite;
		}
		//ri.Printf(PRINT_ALL, "writing %d\n", cmd->picCount + 1);
		qglReadPixels(0, 0, cmd->width, cmd->height, glMode, GL_UNSIGNED_BYTE, fetchBuffer + 18);
		R_GammaCorrect(fetchBuffer + 18, cmd->width * cmd->height * (3 + fetchBufferHasAlpha));
	} else {  // use blur

		if (shotData->allocFailed) {
			ri.Printf(PRINT_ALL, "shotData->allocFailed\n");
		}

		if (shotData->blurTotal && !shotData->allocFailed) {
			if ( shotData->overlapTotal ) {
				int lapIndex = shotData->overlapIndex % shotData->overlapTotal;
				shotData->overlapIndex++;
				/* First frame in a sequence, fill the buffer with the last frames */
				if (shotData->blurIndex == 0) {
					int i, index;
					index = lapIndex;
					//ri.Printf(PRINT_ALL, "first\n");
					accumClearMultiply( shotData->accumAlign, shotData->overlapAlign + (index * shotData->pixelCount/2), shotData->blurMultiply + 0, shotData->pixelCount );
					for (i = 1; i < shotData->overlapTotal; i++) {
						index = (index + 1 ) % shotData->overlapTotal;
						accumAddMultiply( shotData->accumAlign, shotData->overlapAlign + (index * shotData->pixelCount/2), shotData->blurMultiply + i, shotData->pixelCount );
					}
					shotData->blurIndex = shotData->overlapTotal;
				}

				//qglReadPixels(0, 0, cmd->width, cmd->height, glMode, GL_UNSIGNED_BYTE, fetchBuffer + 18);
				qglReadPixels(0, 0, cmd->width, cmd->height, glMode, GL_UNSIGNED_BYTE, (byte *)(shotData->overlapAlign + (lapIndex * shotData->pixelCount/2)));

				//fetchBuffer = cmd->encodeBuffer;
				//FIXME align
				outAlign = (__m64 *)(fetchBuffer + 18);
				//R_GammaCorrect(fetchBuffer + 18, cmd->width * cmd->height * (3 + fetchBufferHasAlpha));
				R_GammaCorrect((byte *)(shotData->overlapAlign + (lapIndex * shotData->pixelCount/2)), cmd->width * cmd->height * (3 + fetchBufferHasAlpha));
				accumAddMultiply( shotData->accumAlign, shotData->overlapAlign + (lapIndex * shotData->pixelCount/2), shotData->blurMultiply + shotData->blurIndex, shotData->pixelCount );
				shotData->blurIndex++;
			} else {  // shotData->overlapTotal
				qglReadPixels(0, 0, cmd->width, cmd->height, glMode, GL_UNSIGNED_BYTE, fetchBuffer + 18);
				R_GammaCorrect(fetchBuffer + 18, cmd->width * cmd->height * (3 + fetchBufferHasAlpha));
				//fetchBuffer = cmd->encodeBuffer;
				//FIXME align
				outAlign = (__m64 *)(fetchBuffer + 18);
				if (shotData->blurIndex == 0) {
					accumClearMultiply( shotData->accumAlign, outAlign, shotData->blurMultiply + 0, shotData->pixelCount );
				} else {
					accumAddMultiply( shotData->accumAlign, outAlign, shotData->blurMultiply + shotData->blurIndex, shotData->pixelCount );
				}
				shotData->blurIndex++;
			}  // shotData->overlapTotal

			if (shotData->blurIndex >= shotData->blurTotal) {
				shotData->blurIndex = 0;
				accumShift( shotData->accumAlign, outAlign, shotData->pixelCount );
				//R_MME_SaveShot( &shotData->shot.main, glConfig.vidWidth, glConfig.vidHeight, shotData->shot.fps, doGamma && !mme_blurGamma->integer, (byte *)outAlign );
				//shotData->frameCount++;
				//ri.Printf(PRINT_ALL, "pic count: %d\n", cmd->picCount);
				if (((cmd->picCount + 1) * blurFrames) % frameRateDivider != 0) {
					goto dontwrite;
				}
			} else {
				// skip saving the shot
				//goto done;
				goto dontwrite;
			}
		}
	}

	if (cmd->tga) {
		byte *buffer;
		int width, height;
		int c;
		int count;

		if (blurFrames > 1) {
			count = cmd->picCount / blurFrames;
		} else {
			count = cmd->picCount;
		}
		count /= frameRateDivider;

		//FIXME hack
		if (shotData == &shotDataLeft) {
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-left-%010d.tga", cmd->givenFileName, count);
		} else {
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-%010d.tga", cmd->givenFileName, count);
		}
		width = cmd->width;
		height = cmd->height;
		buffer = cmd->encodeBuffer;
		Com_Memset (buffer, 0, 18);
        buffer[2] = 2;          // uncompressed type
        buffer[12] = width & 255;
        buffer[13] = width >> 8;
        buffer[14] = height & 255;
        buffer[15] = height >> 8;
        buffer[16] = 24 + 8 * fetchBufferHasAlpha;        // pixel size

		frameSize = cmd->width * cmd->height;

		if (fetchBufferNeedsBGRswap) {
			swap_bgr(buffer + 18, width, height, fetchBufferHasAlpha);
		}

		ri.FS_WriteFile(finalName, buffer, width * height * (3 + fetchBufferHasAlpha) + 18);

		if (shotData == &shotDataLeft) {
			goto done;
		}

		//FIXME no ---  not yet
		if (*ri.SplitVideo) {
			/*  1: (r)  (gb), 2: (r)  (b), 3: (r)  (g), 4: (gb)  (r),
				5: (b)  (r), 6: (g)  (r), 7: (gb) (r)  >7 (gb) (r)
			*/
			memcpy(*ri.ExtraVideoBuffer, buffer, 18 + width * height * (3 + fetchBufferHasAlpha));
			//ri.Printf(PRINT_ALL, "alpha %d\n", fetchBufferHasAlpha);
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-left-%010d.tga", cmd->givenFileName, count);
			c = 18 + width * height * (3 + fetchBufferHasAlpha);

			switch (r_anaglyphMode->integer) {
			case 19:
				// full rgb
				break;
			case 1:
			case 2:
			case 3:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					buffer[i + 0] = buffer[i + 1] = 0;
				}
				break;
			case 4:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					buffer[i + 2] = 0;
				}
				break;
			case 5:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					buffer[i + 1] = buffer[i + 2] = 0;
				}
				break;
			case 6:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					buffer[i + 0] = buffer[i + 2] = 0;
				}
				break;
			case 7:
			default:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					buffer[i + 2] = 0;
				}
				break;
			}

			if (r_anaglyphMode->integer != 19) {
				ri.FS_WriteFile(finalName, buffer, width * height * (3 + fetchBufferHasAlpha) + 18);
			}

			Com_sprintf(finalName, MAX_QPATH, "videos/%s-right-%010d.tga", cmd->givenFileName, count);
			c = 18 + width * height * (3 + fetchBufferHasAlpha);

			switch (r_anaglyphMode->integer) {
			case 19:
				// full rgb
				break;
			case 1:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 2:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 1] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 3:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			default:
				for (i = 18;  i < c;  i += (3 + fetchBufferHasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 1] = 0;
				}
				break;
			}

			ri.FS_WriteFile(finalName, *ri.ExtraVideoBuffer, width * height * (3 + fetchBufferHasAlpha) + 18);
		}
		goto done;
	}

	if (cmd->jpg  ||  cmd->png) {
		byte *buffer;
		int width, height;
		int count;
		const char *type = "png";

		if (cmd->jpg) {
			type = "jpg";
		}

		if (blurFrames > 1) {
			count = cmd->picCount / blurFrames;
		} else {
			count = cmd->picCount;
		}
		count /= frameRateDivider;

		//FIXME hack
		if (shotData == &shotDataLeft) {
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-left-%010d.%s", cmd->givenFileName, count, type);
		} else {
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-%010d.%s", cmd->givenFileName, count, type);
		}

		width = cmd->width;
		height = cmd->height;
		buffer = cmd->captureBuffer + 18;
		ri.FS_WriteFile(finalName, buffer, 1);  // create path
		if (cmd->jpg) {
			if (fetchBufferNeedsBGRswap) {
				swap_bgr(buffer, width, height, fetchBufferHasAlpha);
			}

			if (fetchBufferHasAlpha) {
				convert_rgba_to_rgb(buffer, width, height);
			}

			RE_SaveJPG(finalName, r_jpegCompressionQuality->integer, width, height, buffer, 0);
		} else {  // png
			SavePNG(finalName, buffer, width, height, (3 + fetchBufferHasAlpha));
		}

		if (shotData == &shotDataLeft) {
			goto done;
		}

		if (*ri.SplitVideo) {
			int c;
			const char *type = "png";
			qboolean hasAlpha;

			hasAlpha = fetchBufferHasAlpha;
			if (cmd->jpg) {
				type = "jpg";
				// alpha already removed
				hasAlpha = qfalse;
			}

			/*  1: (r)  (gb), 2: (r)  (b), 3: (r)  (g), 4: (gb)  (r),
				5: (b)  (r), 6: (g)  (r), 7: (gb) (r)  >7 (gb) (r)
			*/
			memcpy(*ri.ExtraVideoBuffer, buffer, width * height * (3 + hasAlpha));

			Com_sprintf(finalName, MAX_QPATH, "videos/%s-left-%010d.%s", cmd->givenFileName, count, type);

			if (r_anaglyphMode->integer != 19) {
				ri.FS_WriteFile(finalName, buffer, 1);  // create path
			}

			//buffer = *ri.ExtraVideoBuffer;

			c = width * height * (3 + hasAlpha);
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
			case 2:
			case 3:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					buffer[i + 1] = buffer[i + 2] = 0;
				}
				break;
			case 4:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					buffer[i + 0] = 0;
				}
				break;
			case 5:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					buffer[i + 0] = buffer[i + 1] = 0;
				}
				break;
			case 6:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					buffer[i + 0] = buffer[i + 2] = 0;
				}
				break;
			case 7:
			default:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					buffer[i + 0] = 0;
				}
				break;
			}

			if (r_anaglyphMode->integer != 19) {
				if (cmd->jpg) {
					RE_SaveJPG(finalName, r_jpegCompressionQuality->integer, width, height, buffer, 0);
				} else {
					SavePNG(finalName, buffer, width, height, (3 + fetchBufferHasAlpha));
				}
			}

			Com_sprintf(finalName, MAX_QPATH, "videos/%s-right-%010d.%s", cmd->givenFileName, count, type);
			ri.FS_WriteFile(finalName, buffer, 1);  // create path

			c = width * height * (3 + hasAlpha);
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 0] = 0;
				}
				break;
			case 2:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 1] = 0;
				}
				break;
			case 3:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			default:
				for (i = 0;  i < c;  i += (3 + hasAlpha)) {
					(*ri.ExtraVideoBuffer)[i + 1] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			}

			if (cmd->jpg) {
				RE_SaveJPG(finalName, r_jpegCompressionQuality->integer, width, height, *ri.ExtraVideoBuffer, 0);
			} else {  // png
				SavePNG(finalName, *ri.ExtraVideoBuffer, width, height, (3 + fetchBufferHasAlpha));
			}
		}
		goto done;
	}

	if( cmd->avi  &&  cmd->motionJpeg )
	{
		if (fetchBufferNeedsBGRswap) {
			swap_bgr(cmd->captureBuffer + 18, cmd->width, cmd->height, fetchBufferHasAlpha);
		}

		if (fetchBufferHasAlpha) {
			convert_rgba_to_rgb(cmd->captureBuffer + 18, cmd->width, cmd->height);
		}

		frameSize = RE_SaveJPGToBuffer(cmd->encodeBuffer + 18, /*FIXME*/ cmd->width * cmd->height * 3, r_jpegCompressionQuality->integer, cmd->width, cmd->height, cmd->captureBuffer + 18, 0);
		if (shotData == &shotDataLeft) {

			ri.CL_WriteAVIVideoFrame(ri.afdLeft, cmd->encodeBuffer + 18, frameSize);
			goto done;
		}
		ri.CL_WriteAVIVideoFrame(ri.afdMain, cmd->encodeBuffer + 18, frameSize);

		if (*ri.SplitVideo) {
			byte *buffer;
			int width, height;
			int c;

			/*  1: (r)  (gb), 2: (r)  (b), 3: (r)  (g), 4: (gb)  (r),
				5: (b)  (r), 6: (g)  (r), 7: (gb) (r)  >7 (gb) (r)
			*/
			width = cmd->width;
			height = cmd->height;
			buffer = cmd->captureBuffer;
			memcpy(*ri.ExtraVideoBuffer, buffer, 18 + width * height * (3 + fetchBufferHasAlpha));

			c = 18 + width * height * 3;
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
			case 2:
			case 3:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 1] = buffer[i + 2] = 0;
				}
				break;
			case 4:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = 0;
				}
				break;
			case 5:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = buffer[i + 1] = 0;
				}
				break;
			case 6:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = buffer[i + 2] = 0;
				}
				break;
			case 7:
			default:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = 0;
				}
				break;
			}

			if (r_anaglyphMode->integer != 19) {
				frameSize = RE_SaveJPGToBuffer(cmd->encodeBuffer + 18, /*FIXME*/ cmd->width * cmd->height * 3, r_jpegCompressionQuality->integer, cmd->width, cmd->height, buffer + 18, 0);
				ri.CL_WriteAVIVideoFrame(ri.afdLeft, cmd->encodeBuffer + 18, frameSize);
			}

			c = 18 + width * height * 3;
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 0] = 0;
				}
				break;
			case 2:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 1] = 0;
				}
				break;
			case 3:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			default:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 1] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			}

			frameSize = RE_SaveJPGToBuffer(cmd->encodeBuffer + 18, /*FIXME*/ cmd->width * cmd->height * 3, r_jpegCompressionQuality->integer, cmd->width, cmd->height, *ri.ExtraVideoBuffer + 18, 0);
			ri.CL_WriteAVIVideoFrame(ri.afdRight, cmd->encodeBuffer + 18, frameSize);
		}
	} else if (cmd->avi) {
		frameSize = cmd->width * cmd->height;
		//byte *buffer;
		int c;
		byte *outBuffer;

		//buffer = cmd->encodeBuffer;
		outBuffer = cmd->encodeBuffer;
		if (fetchBufferHasAlpha  &&  fetchBufferNeedsBGRswap) {
			// gl_rgba
			outBuffer = cmd->captureBuffer;
			//frameSize = cmd->width * cmd->height;
			for (i = 0;  i < frameSize;  i++) {
				outBuffer[i * 3 + 0 + 18] = cmd->encodeBuffer[i * 4 + 2 + 18];
				outBuffer[i * 3 + 1 + 18] = cmd->encodeBuffer[i * 4 + 1 + 18];
				outBuffer[i * 3 + 2 + 18] = cmd->encodeBuffer[i * 4 + 0 + 18];
			}
		} else if (fetchBufferHasAlpha  &&  !fetchBufferNeedsBGRswap) {
			// gl_bgra
			outBuffer = cmd->captureBuffer;
			//frameSize = cmd->width * cmd->height;
			for (i = 0;  i < frameSize;  i++) {
				outBuffer[i * 3 + 0 + 18] = cmd->encodeBuffer[i * 4 + 0 + 18];
				outBuffer[i * 3 + 1 + 18] = cmd->encodeBuffer[i * 4 + 1 + 18];
				outBuffer[i * 3 + 2 + 18] = cmd->encodeBuffer[i * 4 + 2 + 18];
			}
		} else if (fetchBufferNeedsBGRswap) {
			// gl_rbg
			// there is no alpha
			swap_bgr(outBuffer + 18, cmd->width, cmd->height, qfalse);
		} else {
			// it's just gl_bgr, no change needed
		}
		if (shotData == &shotDataLeft) {
			ri.CL_WriteAVIVideoFrame(ri.afdLeft, outBuffer + 18, frameSize * 3);
			goto done;
		}

		ri.CL_WriteAVIVideoFrame(ri.afdMain, outBuffer + 18, frameSize * 3);

		if (*ri.SplitVideo) {
			byte *buffer;
			int width, height;

			/*  1: (r)  (gb), 2: (r)  (b), 3: (r)  (g), 4: (gb)  (r),
				5: (b)  (r), 6: (g)  (r), 7: (gb) (r)  >7 (gb) (r)
			*/

			width = cmd->width;
			height = cmd->height;
			buffer = outBuffer;
			memcpy(*ri.ExtraVideoBuffer, buffer, 18 + width * height * 3);

			c = 18 + width * height * 3;
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
			case 2:
			case 3:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = buffer[i + 1] = 0;
				}
				break;
			case 4:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 2] = 0;
				}
				break;
			case 5:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 1] = buffer[i + 2] = 0;
				}
				break;
			case 6:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 0] = buffer[i + 2] = 0;
				}
				break;
			case 7:
			default:
				for (i = 18;  i < c;  i += 3) {
					buffer[i + 2] = 0;
				}
				break;
			}

			if (r_anaglyphMode->integer != 19) {
				ri.CL_WriteAVIVideoFrame(ri.afdLeft, outBuffer + 18, frameSize * 3);
			}

			c = 18 + width * height * 3;
			switch (r_anaglyphMode->integer) {
			case 19:
				break;
			case 1:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 2:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 1] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 3:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 2] = 0;
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			default:
				for (i = 18;  i < c;  i += 3) {
					(*ri.ExtraVideoBuffer)[i + 0] = (*ri.ExtraVideoBuffer)[i + 1] = 0;
				}
				break;
			}

			ri.CL_WriteAVIVideoFrame(ri.afdRight, *ri.ExtraVideoBuffer + 18, frameSize * 3);
		}
	}

 done:

	if (mme_saveDepth->integer) {  //(cmd->saveDepth) {
		int count;

		if (blurFrames > 1) {
			count = cmd->picCount / blurFrames;
		} else {
			count = cmd->picCount;
		}
		count /= frameRateDivider;

		if (shotData == &shotDataLeft) {
			Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-left-%010d.tga", cmd->givenFileName, count);
		} else {
			if (*ri.SplitVideo  &&  r_anaglyphMode->integer == 19) {
				Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-right-%010d.tga", cmd->givenFileName, count);
			} else if (*ri.SplitVideo  &&  r_anaglyphMode->integer > 0) {
				//FIXME
				//ri.Printf(PRINT_ALL, "FIXME split and r_anaglyphMode != 19\n");
				Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-%010d.tga", cmd->givenFileName, count);
			} else {
				Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-%010d.tga", cmd->givenFileName, count);
			}
		}

		//if ( mme_saveDepth->integer && mme_depthRange->value > 0 ) {
		if (mme_depthRange->value > 0) {
			float focusStart, focusEnd, focusMul;
			float zBase, zAdd, zRange;
			int i;
			GLfloat *out;
			byte *buffer;

			focusStart = mme_depthFocus->value - mme_depthRange->value;
			focusEnd = mme_depthFocus->value + mme_depthRange->value;
			focusMul = 255.0f / (2 * mme_depthRange->value);

			zRange = backEnd.viewParms.zFar - r_znear->value;
            zBase = ( backEnd.viewParms.zFar + r_znear->value ) / zRange;
			zAdd =  ( 2 * backEnd.viewParms.zFar * r_znear->value ) / zRange;

			//outAlign = (__m64 *)(fetchBuffer + 18);
			//outAlign = *ri.Video_DepthBuffer;
			buffer = (byte *)*ri.Video_DepthBuffer;
			buffer += 18;
			out = (GLfloat *)buffer;

			qglDepthRange( 0.0f, 1.0f );
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT, GL_FLOAT, out );

			/* Could probably speed this up a bit with SSE but frack it for now */
			for (i=0;i<shotData->pixelCount;i++) {
				/* Read from the 0 - 1 depth */
				//float zVal = ((float *)outAlign)[i];
				GLfloat zVal = out[i];
				int outVal;
				/* Back to the original -1 to 1 range */
				zVal = zVal * 2.0f - 1.0f;
				/* Back to the original z values */
				zVal = zAdd / ( zBase - zVal );
				/* Clip and scale the range that's been selected */
				if (zVal <= focusStart)
					outVal = 0;
				else if (zVal >= focusEnd)
					outVal = 255;
				else
					outVal = (zVal - focusStart) * focusMul;
				//((byte *)out)[i] = outVal;
				cmd->encodeBuffer[18 + i * 3 + 0] = outVal;
				cmd->encodeBuffer[18 + i * 3 + 1] = outVal;
				cmd->encodeBuffer[18 + i * 3 + 2] = outVal;
			}

			if (cmd->tga) {
				buffer = cmd->encodeBuffer;
				Com_Memset(buffer, 0, 18);
				buffer[2] = 2;          // uncompressed type
				buffer[12] = cmd->width & 255;
				buffer[13] = cmd->width >> 8;
				buffer[14] = cmd->height & 255;
				buffer[15] = cmd->height >> 8;
				buffer[16] = 24;        // pixel size
				//buffer[16] = 8;
				ri.FS_WriteFile(finalName, cmd->encodeBuffer, cmd->width * cmd->height * 3 + 18);
			} else if (cmd->jpg  ||  cmd->png) {
				const char *type = "png";

				if (cmd->jpg) {
					type = "jpg";
				}
				//Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-%010d.jpg", cmd->givenFileName, count);
				if (shotData == &shotDataLeft) {
					Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-left-%010d.%s", cmd->givenFileName, count, type);
				} else {
					if (*ri.SplitVideo  &&  r_anaglyphMode->integer == 19) {
						Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-right-%010d.%s", cmd->givenFileName, count, type);
					} else if (*ri.SplitVideo  &&  r_anaglyphMode->integer > 0) {
						//FIXME
						//ri.Printf(PRINT_ALL, "FIXME split and r_anaglyphMode != 19\n");
						Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-%010d.%s", cmd->givenFileName, count, type);
					} else {
						Com_sprintf(finalName, MAX_QPATH, "videos/%s-depth-%010d.%s", cmd->givenFileName, count, type);
					}
				}

				buffer = cmd->encodeBuffer + 18;
				ri.FS_WriteFile(finalName, buffer, 1);  // create path
				if (cmd->jpg) {
					RE_SaveJPG(finalName, r_jpegCompressionQuality->integer, cmd->width, cmd->height, buffer, 0);
				} else {
					SavePNG(finalName, buffer, cmd->width, cmd->height, 3);
				}
			} else if (cmd->avi  &&  cmd->motionJpeg) {
				//////////////////
				/*
				if (fetchBufferHasAlpha) {
					convert_rgba_to_rgb(cmd->encodeBuffer + 18, cmd->width, cmd->height);
				}
				*/
				frameSize = RE_SaveJPGToBuffer(cmd->captureBuffer + 18, /*FIXME*/ cmd->width * cmd->height * 3, r_jpegCompressionQuality->integer, cmd->width, cmd->height, cmd->encodeBuffer + 18, 0);
				if (shotData == &shotDataLeft) {
					ri.CL_WriteAVIVideoFrame(ri.afdDepthLeft, cmd->captureBuffer + 18, frameSize);
				} else {
					if (*ri.SplitVideo  &&  r_anaglyphMode->integer == 19) {
						ri.CL_WriteAVIVideoFrame(ri.afdDepthRight, cmd->captureBuffer + 18, frameSize);
					} else {
						ri.CL_WriteAVIVideoFrame(ri.afdDepth, cmd->captureBuffer + 18, frameSize);
					}
				}
				//ri.CL_WriteAVIVideoFrame(ri.afdDepth, cmd->captureBuffer + 18, frameSize);
			} else if (cmd->avi) {
				if (shotData == &shotDataLeft) {
					ri.CL_WriteAVIVideoFrame(ri.afdDepthLeft, cmd->encodeBuffer + 18, cmd->width * cmd->height * 3);
				} else {
					if (*ri.SplitVideo  &&  r_anaglyphMode->integer == 19) {
						ri.CL_WriteAVIVideoFrame(ri.afdDepthRight, cmd->encodeBuffer + 18, cmd->width * cmd->height * 3);
					} else if (*ri.SplitVideo  &&  r_anaglyphMode->integer > 0) {
						//FIXME
						//ri.Printf(PRINT_ALL, "FIXME split and r_anaglyphMode != 19\n");
						ri.CL_WriteAVIVideoFrame(ri.afdDepth, cmd->encodeBuffer + 18, cmd->width * cmd->height * 3);
					} else {
						ri.CL_WriteAVIVideoFrame(ri.afdDepth, cmd->encodeBuffer + 18, cmd->width * cmd->height * 3);
					}
				}
				//ri.CL_WriteAVIVideoFrame(ri.afdDepth, cmd->encodeBuffer + 18, cmd->width * cmd->height * 3);
			}
		}
	}

 dontwrite:
	//ri.Hunk_FreeTempMemory( outAlloc );
	//shotData->shot.take = qfalse;

	return (const void *)(cmd + 1);
}

