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

	/* TODO: how to make it so that there's not all these fields? */
	i32 transition_tile_type_to; /* NOTE(josh): ignored unless tile type is TILE_TYPE_TRANSITIONING */
	i32 transition_tile_type_from; /* NOTE(josh): ignored unless tile type is TILE_TYPE_TRANSITIONING */
	i32 transition_unit_type_to; /* NOTE(josh): ignored unless tile type is UNIT_TYPE_TRANSITIONING */
	i32 transition_unit_type_from; /* NOTE(josh): ignored unless tile type is UNIT_TYPE_TRANSITIONING*/
} tile;
