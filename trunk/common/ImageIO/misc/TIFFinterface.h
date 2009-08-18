
namespace ImageIO
{

namespace {

	tsize_t tiffReadProc(thandle_t handle, tdata_t buf, tsize_t size)
	{
		IFile* pfile = (IFile*)handle;
		DWORD nBytesRead;
		pfile->Read(buf, size, nBytesRead);
		return nBytesRead;
	}

	tsize_t tiffWriteProc(thandle_t handle, tdata_t buf, tsize_t size)
	{
		return 0;
	}

	toff_t tiffSeekProc(thandle_t handle, toff_t off, int whence)
	{
		IFile* pfile = (IFile*)handle;
		return pfile->Seek(off, 0);
	}

	int tiffCloseProc(thandle_t fd)
	{
		return 0;
	}

	toff_t tiffSizeProc(thandle_t handle)
	{
		IFile* pfile = (IFile*)handle;
		return pfile->Size();
	}

	int tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
	{
		return 0;
	}

	void tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
	{
	}

} // namespace

TIFF* OpenTiff(IFile* pFile)
{
	TIFF* ptiff = TIFFClientOpen(
		"",
		"r",
		(thandle_t)pFile,
		tiffReadProc,
		tiffWriteProc,
		tiffSeekProc,
		tiffCloseProc,
		tiffSizeProc,
		tiffMapProc,
		tiffUnmapProc
	);
    if (ptiff)
		ptiff->tif_fd = (long)pFile;
	return ptiff;
}

} // namespace ImageIO

