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
