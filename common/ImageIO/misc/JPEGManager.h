#pragma once

#include "File.h"
#include "MemoryFile.h"

namespace ImageIO
{

// clone of CxImage's JPEGManager
template <size_t BUFFER_SIZE = 4096>
class JPEGManager : public jpeg_destination_mgr, public jpeg_source_mgr
{
public:
	
	JPEGManager(IFile* pFile)
	{
        pFile_ = pFile;

		init_destination = InitDestination;
		empty_output_buffer = EmptyOutputBuffer;
		term_destination = TermDestination;
		
		init_source = InitSource;
		
//		if (pFile_->HasBuffer()) {
//			fill_input_buffer = FillInputBuffer_JustPass;
//		}else {
			pBuffer_ = new unsigned char[BUFFER_SIZE];
			fill_input_buffer = FillInputBuffer;
//		}
		skip_input_data = SkipInputData;
		resync_to_restart = jpeg_resync_to_restart; // use default method
		term_source = TermSource;
		next_input_byte = NULL; //* => next byte to read from buffer 
		bytes_in_buffer = 0;	//* # of bytes remaining in buffer 

	}
	~JPEGManager()
	{
//		if (!pFile_->HasBuffer()) {
			delete [] pBuffer_;
//		}
	}

	static void InitDestination(j_compress_ptr cinfo)
	{
		JPEGManager* pDest = static_cast<JPEGManager*>(cinfo->dest);
		pDest->next_output_byte = pDest->pBuffer_;
		pDest->free_in_buffer = BUFFER_SIZE;
	}

	static boolean EmptyOutputBuffer(j_compress_ptr cinfo)
	{
		JPEGManager* pDest = static_cast<JPEGManager*>(cinfo->dest);
		DWORD bytesWritten;
		if (!pDest->pFile_->Write(pDest->pBuffer_,BUFFER_SIZE,bytesWritten) || bytesWritten != BUFFER_SIZE)
			ERREXIT(cinfo, JERR_FILE_WRITE);
		pDest->next_output_byte = pDest->pBuffer_;
		pDest->free_in_buffer = BUFFER_SIZE;
		return TRUE;
	}

	static void TermDestination(j_compress_ptr cinfo)
	{
		JPEGManager* pDest = static_cast<JPEGManager*>(cinfo->dest);
		size_t datacount = BUFFER_SIZE - pDest->free_in_buffer;
		/* Write any data remaining in the buffer */
		if (datacount > 0) {
			DWORD bytesWritten;
			if (!pDest->pFile_->Write(pDest->pBuffer_,datacount,bytesWritten) || bytesWritten != datacount)
				ERREXIT(cinfo, JERR_FILE_WRITE);
		}
		pDest->pFile_->Flush();
		/* Make sure we wrote the output file OK */
//		if (pDest->pFile_->Error()) ERREXIT(cinfo, JERR_FILE_WRITE);
		return;
	}

	static void InitSource(j_decompress_ptr cinfo)
	{
		JPEGManager* pSource = static_cast<JPEGManager*>(cinfo->src);
		pSource->bStartOfFile_ = TRUE;
	}

	static boolean FillInputBuffer(j_decompress_ptr cinfo)
	{
		DWORD nbytes;
		JPEGManager* pSource = static_cast<JPEGManager*>(cinfo->src);
		pSource->pFile_->Read(pSource->pBuffer_,BUFFER_SIZE,nbytes);
		if (nbytes <= 0){
			if (pSource->bStartOfFile_)	//* Treat empty input file as fatal error 
				ERREXIT(cinfo, JERR_INPUT_EMPTY);
			WARNMS(cinfo, JWRN_JPEG_EOF);
			// Insert a fake EOI marker 
			pSource->pBuffer_[0] = (JOCTET) 0xFF;
			pSource->pBuffer_[1] = (JOCTET) JPEG_EOI;
			nbytes = 2;
		}
		pSource->next_input_byte = pSource->pBuffer_;
		pSource->bytes_in_buffer = nbytes;
		pSource->bStartOfFile_ = FALSE;
		return TRUE;
	}

	static boolean FillInputBuffer_JustPass(j_decompress_ptr cinfo)
	{
		JPEGManager* pSource = static_cast<JPEGManager*>(cinfo->src);
		IFile* pFile = pSource->pFile_;
		DWORD curPos = pFile->Tell();
		pSource->pBuffer_ = (unsigned char*)pFile->GetBuffer() + curPos;
		DWORD newPos = pFile->Seek(BUFFER_SIZE, FILE_CURRENT);
		DWORD nbytes = newPos - curPos;
		if (nbytes <= 0){
			if (pSource->bStartOfFile_)	//* Treat empty input file as fatal error 
				ERREXIT(cinfo, JERR_INPUT_EMPTY);
			WARNMS(cinfo, JWRN_JPEG_EOF);
			// Insert a fake EOI marker 
			pSource->pBuffer_[0] = (JOCTET) 0xFF;
			pSource->pBuffer_[1] = (JOCTET) JPEG_EOI;
			nbytes = 2;
		}
		pSource->next_input_byte = pSource->pBuffer_;
		pSource->bytes_in_buffer = nbytes;
		pSource->bStartOfFile_ = FALSE;
		return TRUE;
	}

	static void SkipInputData(j_decompress_ptr cinfo, long num_bytes)
	{
		JPEGManager* pSource = static_cast<JPEGManager*>(cinfo->src);
		if (num_bytes > 0){
			while (num_bytes > (long)pSource->bytes_in_buffer){
				num_bytes -= (long)pSource->bytes_in_buffer;
				pSource->fill_input_buffer(cinfo);
				// note we assume that fill_input_buffer will never return FALSE,
				// so suspension need not be handled.
			}
			pSource->next_input_byte += (size_t) num_bytes;
			pSource->bytes_in_buffer -= (size_t) num_bytes;
		}
	}

	static void TermSource(j_decompress_ptr cinfo)
	{
		return;
	}
protected:
	IFile* pFile_;
	unsigned char *pBuffer_;
	bool bStartOfFile_;
};

}	// namespace ImageIO

