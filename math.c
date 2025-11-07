#define PI 3.14159265f

#define ROUND_FLOAT_TO_FLOAT(x) (f32)( (i32)(x + 0.5f) )
#define FLOAT_EQUALS(x, y) ( ((x - y) < 0.01f) && ((x - y) > -0.01f))

typedef struct {
	f32 x;
	f32 y;
} vector_2;

typedef struct {
	f32 m11;
	f32 m12;
	f32 m21;
	f32 m22;
} matrix_2x2;

vector_2 multiply_vector_2_by_matrix_2x2(
		vector_2 vector, 
		matrix_2x2 matrix)
{
	vector_2 result;

	result.x = vector.x * matrix.m11 + vector.y * matrix.m21;
	result.y = vector.x * matrix.m12 + vector.y * matrix.m22;
	return result;
}

/* subtract vec2 from vec1 */
vector_2 sub_vec2(vector_2 vec1, vector_2 vec2)
{
	vector_2 result;
	result.x = vec1.x - vec2.x;
	result.y = vec1.y - vec2.y;
	return result;
}

f32 det_2x2_from_vectors(vector_2 c1, vector_2 c2)
{
	f32 result = (c1.x * c2.y) - (c2.x * c1.y);
	return result;
}

f32 det_2x2_from_matrix(matrix_2x2)
{
	f32 result = 0.0f;
	/* TODO: */
	return result;
}
