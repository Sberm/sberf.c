/*═══════════════════════════════════════════════════════════════════════╗
║                          ©  Howard Chu                                 ║
║                                                                        ║
║ Permission to use, copy, modify, and/or distribute this software for   ║
║ any purpose with or without fee is hereby granted, provided that the   ║
║ above copyright notice and this permission notice appear in all copies ║
╚═══════════════════════════════════════════════════════════════════════*/

#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include "stack.h"
#include "plot.h"
#include "util.h"
#include "comm.h"

extern int symbolize(pid_t pid, unsigned long long addr, char *buf, size_t len);

/*
 * Don't include definition of sym.h, because it is included in record.c therefore multiple
 * definition
 */
#define SYM_H_NO_DEF 
#include "sym.h"

int color_index = 0;
const int *color_palette;
int color_palette_sz;

char* svg_str;
int svg_sz = 65536;
int svg_index = 0;

#define MAX_WIDTH 1200.0
double max_height = 0;

// x start
#define X_ST 10.0

#define FRAME_HEIGHT 15.4
#define Y_MARGIN 0.5

enum PLOT_MODE {
	PLOT_CYCLE,
	PLOT_OFF_CPU,
} plot_mode;

static const int blue[] = {
	0x42E5CA,
	0x5395E5,
	0x69E5AA,
	0x67BAE6,
	0x2DC5E5,
};

static const int pink[] = {
	0xFEE3EC,
	0xF9C5D5,
	0xF999B7,
	0xF2789F,
};

static const int flame[] = {
	0xFFF78A,
	0xFFE382,
	0xFFC47E,
	0xFFAD84,
};

static const char css[] = "<style type='text/css'>\n"
"text { font-size:11px; fill:rgb(0,0,0);}\n"
".hide {display: none;}\n"
"#frames > *:hover { stroke:black; stroke-width:0.5; cursor:pointer; }\n"
"</style>\n";

static const char js[] = "<script><![CDATA[\n"
"const svg = document.querySelector('svg');\n"
"var frames;\n"
"const fullWidth = svg.viewBox.baseVal.width - 18;\n"
"\n"
"Number.prototype.greaterThanEqual = function (o) {\n"
"    return this.valueOf() - o.valueOf() > -0.1;\n"
"};\n"
"\n"
"Number.prototype.lessThanEqual = function (o) {\n"
"    return o.valueOf() - this.valueOf() > -0.1;\n"
"};\n"
"\n"
"Number.prototype.eq = function (o) {\n"
"    return Math.abs(o.valueOf() - this.valueOf()) < 0.1;\n"
"};\n"
"\n"
"Number.prototype.gte = Number.prototype.greaterThanEqual;\n"
"Number.prototype.lte = Number.prototype.lessThanEqual;\n"
"\n"
"function renderText(gi) {\n"
"	let rect = gi.querySelector('rect');\n"
"	let title = gi.querySelector('title');\n"
"	let content = title.textContent;\n"
"	let text = gi.querySelector('text');\n"
"	let width = rect.width.baseVal.value;\n"
"\n"
"	text.textContent = content;\n"
"	if (text.getSubStringLength(0, content.length) <= width) {\n"
"		return;\n"
"	}\n"
"	if (width < 11.5) {\n"
"		text.textContent = '';\n"
"		return;\n"
"	}\n"
"	for (let i = 0; i < content.length; i += 2) {\n"
"		if (text.getSubStringLength(0, i + 2) > width) {\n"
"			text.textContent = content.substring(0, i);\n"
"			return;\n"
"		}\n"
"	}\n"
"	text.innerHTML = '';\n"
"}\n"
"\n"
"function main(evt) {\n"
"	frames = document.getElementById('frames').querySelectorAll('g');\n"
"	let g = document.querySelectorAll('g');\n"
"	g.forEach((gi) => {renderText(gi)});\n"
"}\n"
"\n"
"function getToUpdate(f) {\n"
"	let parents = [], children = [];\n"
"\n"
"	const rect = f.querySelector('rect');\n"
"	\n"
"	const width = rect.width.baseVal.value;\n"
"	const y = rect.y.baseVal.value;\n"
"	\n"
"	const left = rect.x.baseVal.value;\n"
"	const right = left + width;\n"
"\n"
"	let rect_, width_, y_, left_, right_;\n"
"\n"
"	for (let i = 0; i < frames.length; i++) {\n"
"		rect_ = frames[i].querySelector('rect');\n"
"		width_ = rect_.width.baseVal.value;\n"
"		y_ = rect_.y.baseVal.value;\n"
"		left_ = rect_.x.baseVal.value;\n"
"		right_ = left_ + width_;\n"
"\n"
"		if (left_.lte(left) && right_.gte(right) && y_ < y)\n"
"			parents.push(frames[i]);\n"
"		else if (left_.gte(left) && right_.lte(right) && y_ > y)\n"
"			children.push(frames[i]);\n"
"		else\n"
"			frames[i].classList.add('hide');\n"
"	}\n"
"\n"
"	f.classList.remove('hide');\n"
"\n"
"	return {\n"
"		parents: parents,\n"
"		children: children,\n"
"	};\n"
"}\n"
"\n"
"function zoomIn(frames_, ratio = null, targetLeft = null) {\n"
"	if (ratio === null) { // parent\n"
"		for (let i = 0; i < frames_.length; i++) {\n"
"			let rect = frames_[i].querySelector('rect');\n"
"			rect.setAttribute('x', 10);\n"
"			rect.setAttribute('width', fullWidth);\n"
"\n"
"			let text = frames_[i].querySelector('text');\n"
"			text.setAttribute('x', 10);\n"
"		}\n"
"	} else { // child\n"
"		for (let i = 0; i < frames_.length; i++) {\n"
"			let rect = frames_[i].querySelector('rect');\n"
"			let d = rect.x.baseVal.value - targetLeft;\n"
"			rect.setAttribute('x', d * ratio + 10);\n"
"			rect.setAttribute('width', rect.width.baseVal.value * ratio);\n"
"\n"
"			let text = frames_[i].querySelector('text');\n"
"			text.setAttribute('x', d * ratio + 10);\n"
"		}\n"
"	}\n"
"}\n"
"\n"
"// click to expand\n"
"window.addEventListener('click', function(e) {\n"
"	let target = e.target;\n"
"\n"
"	if (target.tagName !== 'g')\n"
"		target = target.parentElement;\n"
"	\n"
"	if (target === null)\n"
"		return;\n"
"\n"
"	let rect = target.querySelector('rect');\n"
"	let text = target.querySelector('text');\n"
"\n"
"	let oldWidth = rect.width.baseVal.value;\n"
"	let oldLeft = rect.x.baseVal.value;\n"
"	let ratio = fullWidth / oldWidth;\n"
"\n"
"	const {parents, children} = getToUpdate(target);\n"
"\n"
"	// set itself to full width\n"
"	rect.setAttribute('width', fullWidth);\n"
"	rect.setAttribute('x', 10);\n"
"	text.setAttribute('x', 10);\n"
"\n"
"	// parent\n"
"	zoomIn(parents);\n"
"\n"
"	// children\n"
"	zoomIn(children, ratio, oldLeft);\n"
"\n"
"	// re-render\n"
"	[...parents, ...children, target].forEach((f) => {renderText(f)});\n"
"}, false)\n"
"]]></script>\n";

void __plot(struct stack_ag* p, unsigned long long p_cnt, double x, double y, double len, int depth, struct ksyms* ksym_tb, struct usyms* usym_tb)
{
	if (p == NULL)
		return;

	double width = ((double)p->cnt / (double)p_cnt) * len;
	double height = FRAME_HEIGHT;
	int c = color_palette[color_index];
	char frame_title[128] = { [0] = '\0' };
	char g_str[1024];

	color_index = color_index + 1 > color_palette_sz - 1 ? 0 : color_index + 1;

	if (p->addr == 0 && !p->is_comm) {
		strcpy(frame_title, "all");
	} else if (p->is_comm) {
		strcpy(frame_title, p->comm);
	} else {
		// addr_to_sym(ksym_tb, usym_tb, p->addr, frame_title);
		symbolize(1445742, p->addr, (char *)frame_title, sizeof(frame_title));
	}

	switch (plot_mode) {
	case PLOT_CYCLE:
		snprintf(g_str, sizeof(g_str), "<g>\n"
		 	 "<title>%s (%%%.2f)</title><rect x='%.2f' y='%.2f'"
		 	 " width='%.2f' height='%.2f' fill='#%06x'"
		 	 " rx='1' ry='1' />\n"
		 	 "<text  x='%.2f' y='%.2f' ></text>\n"
		 	 "</g>\n",
		 	 frame_title, width / MAX_WIDTH * 100, x, y, width, height,
		 	 c, x + 0.2, y + FRAME_HEIGHT - 4);
		break;

	case PLOT_OFF_CPU:
		snprintf(g_str, sizeof(g_str), "<g>\n"
			 "<title>%s (%.3fs)</title><rect x='%.2f' y='%.2f'"
			 " width='%.2f' height='%.2f' fill='#%06x'"
			 " rx='1' ry='1' />\n"
			 "<text  x='%.2f' y='%.2f' ></text>\n"
			 "</g>\n",
			 frame_title, ((double)p->cnt / 1000000000ULL), x, y, width, height,
			 c, x + 0.2, y + FRAME_HEIGHT - 4);
		break;

	default:
		break;
	}

	/* realloc just like a stl vector */
	if (svg_index + strlen(g_str) >= svg_sz) {
		svg_sz *= 2;
		svg_str = realloc(svg_str, sizeof(char) * svg_sz);
	}

	strcpy(svg_str + svg_index , g_str);
	svg_index += strlen(g_str);

	__plot(p->next, p_cnt, x + width, y, len, depth, ksym_tb, usym_tb);
	__plot(p->child, p->cnt, x, y + FRAME_HEIGHT + Y_MARGIN, width, depth + 1, ksym_tb, usym_tb);
}

int plot_off_cpu(struct stack_ag *p, char* file_name, struct comm_pids *comms)
{
	struct ksyms* ksym_tb;
	struct usyms* usym_tb;
	int *pids;
	pthread_t loading_thread;
	struct loading_args la = {
		.str = "loading symbols",
		.dot = '.',
	};

	if (p == NULL)
		return -1;

	max_height = stack_get_depth(p) * FRAME_HEIGHT;

	pthread_create(&loading_thread, NULL, print_loading, (void *)&la);

	pids = malloc(comms->nr * sizeof(pid_t));
	if (pids == NULL) {
		printf("Failed to create pid array");
		return -1;
	}

	ksym_tb = ksym_load();
	usym_tb = usym_load(pids, comms->nr);
	if (ksym_tb == NULL || usym_tb == NULL) {
	    printf("Failed to load symbols when plotting\n");
		return -1;
	}

	pthread_cancel(loading_thread);

	printf("\nloaded.\n");
	
	FILE* fp = fopen(file_name, "w");

	svg_str = malloc(sizeof(char) * svg_sz);
	memset(svg_str, 0, sizeof(char) * svg_sz);

	if (svg_str == NULL) {
		printf("Failed to allocate memory for writing svg\n");
		fclose(fp);
		return -1;
	}

	color_palette = pink;
	color_palette_sz = ARRAY_LEN(pink);
	
	plot_mode = PLOT_OFF_CPU;
	
	/* write svg to svg_str */
	__plot(p, p->cnt, X_ST, 0, MAX_WIDTH, 0, ksym_tb, usym_tb);

	fprintf(fp, "<svg version='1.1' width='%.0f' height='%.0f' "
		"onload='main(evt)' viewBox='0 0 %.0f %.0f' xmlns='http://www.w3.org/2000/svg' "
		"xmlns:xlink='http://www.w3.org/1999/xlink'>\n",
		MAX_WIDTH + 5, max_height + 5, MAX_WIDTH + 5, max_height + 5);

	fputs(css, fp);
	fputs(js, fp);
	fputs(svg_str, fp); // use fprintf here will cause seg fault

	fprintf(fp, "</svg>\n");

cleanup:
	free(svg_str);
	free(pids);
	fclose(fp);
	ksym_free(ksym_tb);
	usym_free(usym_tb);

	return 0;
}

int plot(struct stack_ag *p, char* file_name, struct comm_pids *comms)
{
	struct ksyms* ksym_tb;
	struct usyms* usym_tb;
	int *pids;
	pthread_t loading;
	struct loading_args la = {
		.str = "loading symbols",
		.dot = '.',
	};

	if (p == NULL)
		return -1;

	max_height = stack_get_depth(p) * FRAME_HEIGHT;

	pthread_create(&loading, NULL, print_loading, (void *)&la);

	pids = malloc(comms->nr * sizeof(pid_t));
	if (pids == NULL) {
		printf("Failed to create pid array");
		return -1;
	}

	ksym_tb = ksym_load();
	usym_tb = usym_load(pids, comms->nr);
	if (ksym_tb == NULL || usym_tb == NULL) {
	    printf("Failed to load symbols when plotting\n");
		return -1;
	}

	pthread_cancel(loading);

	printf("\nloaded.\n");
	
	FILE* fp = fopen(file_name, "w");

	svg_str = malloc(sizeof(char) * svg_sz);
	memset(svg_str, 0, sizeof(char) * svg_sz);

	if (svg_str == NULL) {
		printf("Failed to allocate memory for writing svg\n");
		fclose(fp);
		return -1;
	}

	color_palette = flame;
	color_palette_sz = ARRAY_LEN(flame);

	plot_mode = PLOT_CYCLE;
	
	__plot(p, p->cnt, X_ST, 0, MAX_WIDTH, 0, ksym_tb, usym_tb);

	fprintf(fp, "<svg version='1.1' width='%.0f' height='%.0f' "
		"onload='main(evt)' viewBox='0 0 %.0f %.0f' "
		"xmlns='http://www.w3.org/2000/svg' "
		"xmlns:xlink='http://www.w3.org/1999/xlink'>\n", 
		MAX_WIDTH + 18, max_height + 18, MAX_WIDTH + 18, max_height + 18);

	fputs(css, fp);
	fputs(js, fp);
	fputs("<g id='frames'>", fp); // id=frames
	fputs(svg_str, fp); // use fprintf here will cause SEGV
	fputs("</g>", fp); // id=frames

	fprintf(fp, "</svg>\n");

cleanup:
	free(svg_str);
	free(pids);
	fclose(fp);
	ksym_free(ksym_tb);
	usym_free(usym_tb);

	return 0;
}
