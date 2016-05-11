// reader_test.c : run automatic tests on the pdf/raster reader library
//

#include <stdlib.h>
#include <stdio.h>
#include "pdfrasread_files.h"
#ifdef WIN32
#include <direct.h>
#define getcwd(buff,thing) _getcwd(buff,thing)
#else
#include <unistd.h>
#endif

#include "test_support.h"

void signature_tests()
{
	printf("--signature recognition\n");
	// check that it works with files
	ASSERT(0 == pdfrasread_recognize_file(NULL));
	ASSERT(0 == pdfrasread_recognize_filename("exists.not"));
	ASSERT(0 == pdfrasread_recognize_filename("badsig1.pdf"));
	ASSERT(0 == pdfrasread_recognize_filename("badsig2.pdf"));
	ASSERT(1 == pdfrasread_recognize_filename("valid1.pdf"));
	// bad PDF, but has PDF/raster signature:
	ASSERT(1 == pdfrasread_recognize_filename("bad_trailer1.pdf"));
	printf("passed\n");
}

static size_t freader(void *source, pduint32 offset, size_t length, char *buffer)
{
	FILE* f = (FILE*)source;
	if (0 != fseek(f, offset, SEEK_SET)) {
		return 0;
	}
	return fread(buffer, sizeof(pduint8), length, f);
}

static void fcloser(void* source)
{
	if (source) {
		FILE* f = (FILE*)source;
		fclose(f);
	}
}


void create_destroy_tests()
{
	printf("-- reader create/destroy tests --\n");
	t_pdfrasreader* reader = pdfrasread_create(PDFRAS_API_LEVEL, &freader, &fcloser);
	ASSERT(reader != NULL);
	// a freshly created reader is not open:
	ASSERT(!pdfrasread_is_open(reader));
	// closing a reader that is not open does nothing and returns FALSE:
	ASSERT(pdfrasread_close(reader) == FALSE);
	// the source of a newly created reader is 0 (NULL)
	ASSERT(pdfrasread_source(reader) == NULL);

	pdfrasread_destroy(reader);
	printf("passed\n");
}


void page_count_tests()
{
	printf("--page counting --\n");
	// some random PDF with 3 pages, old-style xref table:
	ASSERT(3 == pdfrasread_page_count_filename("3pages.pdf"));
	// generated by pdfras_writer:
	ASSERT(6 == pdfrasread_page_count_filename("valid1.pdf"));
	ASSERT(-1 == pdfrasread_page_count_filename("bad_trailer1.pdf"));
	ASSERT(-1 == pdfrasread_page_count_filename("badxref1.pdf"));
	printf("passed\n");
}

void page_info_tests()
{
	printf("--page info--\n");
	t_pdfrasreader* reader = pdfrasread_open_filename(PDFRAS_API_LEVEL, "valid1.pdf");
	// that should be successful:
	ASSERT(reader != NULL);
	// After successful open, this must be TRUE:
	ASSERT(pdfrasread_is_open(reader)==TRUE);
	// don't know what source is exactly, but assuming it's a FILE*
	// it shouldn't be NULL:
	ASSERT(pdfrasread_source(reader) != NULL);
	// that file has 6 pages:
	ASSERT(6 == pdfrasread_page_count(reader));

	ASSERT(PDFRAS_GRAY8 == pdfrasread_page_format(reader, 0));
	ASSERT(8 == pdfrasread_page_width(reader, 0));
	ASSERT(11 == pdfrasread_page_height(reader, 0));
	ASSERT(0 == pdfrasread_page_rotation(reader, 0));
	ASSERT(2.0 == pdfrasread_page_horizontal_dpi(reader, 0));
	ASSERT(2.0 == pdfrasread_page_vertical_dpi(reader, 0));

	ASSERT(PDFRAS_GRAY16 == pdfrasread_page_format(reader, 1));
	ASSERT(64 == pdfrasread_page_width(reader, 1));
	ASSERT(512 == pdfrasread_page_height(reader, 1));
	ASSERT(0 == pdfrasread_page_rotation(reader, 1));
	ASSERT(16.0 == pdfrasread_page_horizontal_dpi(reader, 1));
	ASSERT(128.0 == pdfrasread_page_vertical_dpi(reader, 1));

	ASSERT(PDFRAS_BITONAL == pdfrasread_page_format(reader, 2));
	ASSERT(850 == pdfrasread_page_width(reader, 2));
	ASSERT(1100 == pdfrasread_page_height(reader, 2));
	ASSERT(0 == pdfrasread_page_rotation(reader, 2));
	ASSERT(100.0 == pdfrasread_page_horizontal_dpi(reader, 2));
	ASSERT(100.0 == pdfrasread_page_vertical_dpi(reader, 2));

	ASSERT(PDFRAS_BITONAL == pdfrasread_page_format(reader, 3));
	ASSERT(2521 == pdfrasread_page_width(reader, 3));
	ASSERT(3279 == pdfrasread_page_height(reader, 3));
	ASSERT(0 == pdfrasread_page_rotation(reader, 3));
	ASSERT(300.0 == pdfrasread_page_horizontal_dpi(reader, 3));
	ASSERT(300.0 == pdfrasread_page_vertical_dpi(reader, 3));

	ASSERT(PDFRAS_RGB24 == pdfrasread_page_format(reader, 4));
	ASSERT(175 == pdfrasread_page_width(reader, 4));
	ASSERT(100 == pdfrasread_page_height(reader, 4));
	ASSERT(90 == pdfrasread_page_rotation(reader, 4));
	ASSERT(50.0 == pdfrasread_page_horizontal_dpi(reader, 4));
	ASSERT(50.0 == pdfrasread_page_vertical_dpi(reader, 4));

	ASSERT(PDFRAS_RGB24 == pdfrasread_page_format(reader, 5));
	ASSERT(850 == pdfrasread_page_width(reader, 5));
	ASSERT(1100 == pdfrasread_page_height(reader, 5));
	ASSERT(180 == pdfrasread_page_rotation(reader, 5));
	ASSERT(100.0 == pdfrasread_page_horizontal_dpi(reader, 5));
	ASSERT(100.0 == pdfrasread_page_vertical_dpi(reader, 5));

	pdfrasread_destroy(reader);
	printf("passed\n");
} // page_info_tests

void strip_data_tests()
{
	printf("-- strip data tests --\n");
	// open our standard test file
	t_pdfrasreader* reader = pdfrasread_open_filename(PDFRAS_API_LEVEL, "valid1.pdf");
	ASSERT(reader != NULL);
	ASSERT(pdfrasread_is_open(reader) == TRUE);
	int pages = pdfrasread_page_count(reader);
	// check out page 0
	int p = 0;
	// get the number of strips on page 0
	int page_height = pdfrasread_page_height(reader, p);
	int total_height = 0;
	int strips = pdfrasread_strip_count(reader, p);
	// get the maximum buffer size needed for any strip on this page
	size_t max_size = pdfrasread_max_strip_size(reader, p);
	pduint8* rawstrip = (pduint8*)malloc(max_size);
	ASSERT(rawstrip != NULL);
	for (int s = 0; s < strips; s++) {
		//int h = pdfrasread_strip_height(reader, p, s);
		//ASSERT(h > 0);
		//total_height += h;
		ASSERT(total_height <= page_height);
		size_t rcvd = pdfrasread_read_raw_strip(reader, p, s, rawstrip, max_size);
		ASSERT(rcvd <= max_size);
	}
	//ASSERT(total_height == page_height);
	free(rawstrip);
	printf("passed\n");
} // strip_data_tests


int main(int argc, char* argv[])
{
	printf("pdfraster reader_test\n");
	char* cwd = getcwd(NULL, 0);
	printf("cwd: %s\n", cwd);
	free(cwd);
	signature_tests();
	create_destroy_tests();
	page_count_tests();
	page_info_tests();
	strip_data_tests();

	unsigned fails = get_number_of_failures();

	printf("\n------------------------------\n");
	printf("%u fails.  Hit [enter] to exit:\n", fails);
	getchar();
	return fails;
}

