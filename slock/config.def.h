/* user and group to drop privileges to */
static const char *user  = "nobody";
static const char *group = "nobody"; // use "nobody" for arch

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#272822",   /* during input */
	[FAILED] = "#161613",   /* wrong password */
	[CAPS] =   "#58d9ef",       /* CapsLock on */
	[BLOCKS] = "#e6db69",   /* key feedback block */
};

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 1;

static const secretpass scom[] = {
	/* Password             command */
	{ "12345",           "doas poweroff"},
};

static short int blocks_enabled = 1; // 0 = don't show blocks
static const int blocks_width = 0; // 0 = full width
static const int blocks_height = 16;

// position
static const int blocks_x = 0;
static const int blocks_y = 0;

// Number of blocks
static const int blocks_count = 10;

