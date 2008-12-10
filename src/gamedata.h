

struct dot {
	int x;				// x coordinate of dot
	int y;				// y coordinate of dot
	int ndots;			// Number of neighboring dots
	int dots[8];		// neighboring dots
	int nsquares;		// number of squares touching this dot
	int sq[8];			// squares touching dot
	int nlines;			// number of lines touching dot
	int lin[8];			// lines touching dot
};

struct square {
	int number;			// Number inside the square
	int dots[4];		// dots associated to square
	int lines[4];		// lines associated to square
};

struct line {
	int state;			// State of line
	int dots[2];		// Dots at ends of line
};

struct game {
	int ndots;				// Number of dots
	struct dot *dots;		// List of dots
	int nsquares;			// Number of squares
	struct square *squares;	// List of squares
	int nlines;
	struct line *lines;
	int sq_width;
	int sq_height;
};
