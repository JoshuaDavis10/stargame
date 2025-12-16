#define MAX_TILEMAP_WIDTH 7 
#define MAX_TILEMAP_HEIGHT 7

enum {
	TILE_TYPE_BLUE,
	TILE_TYPE_RED,
	TILE_TYPE_GREEN,
	TILE_TYPE_TRANSITIONING,
	TILE_TYPE_COUNT
};

enum {
	UNIT_TYPE_NONE,
	UNIT_TYPE_BLUE,
	UNIT_TYPE_RED,
	UNIT_TYPE_GREEN,
	UNIT_TYPE_TRANSITIONING,
	UNIT_TYPE_COUNT
};

typedef struct {
	i32 tile_type;
	i32 unit_type;

	/* TODO: alternative struct for the data that actually gets written to file, which would just be tile_type + unit_type */
	/* call it like save_tile or smn */
	i32 transition_tile_type_to; /* NOTE(josh): ignored unless tile type is TILE_TYPE_TRANSITIONING */
	i32 transition_tile_type_from; /* NOTE(josh): ignored unless tile type is TILE_TYPE_TRANSITIONING */
	i32 transition_unit_type_to; /* NOTE(josh): ignored unless tile type is UNIT_TYPE_TRANSITIONING */
	i32 transition_unit_type_from; /* NOTE(josh): ignored unless tile type is UNIT_TYPE_TRANSITIONING*/
} tile;
