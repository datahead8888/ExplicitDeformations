#pragma once

const int DIMENSION = 3;	//Number of dimensions - 3 means 3D

//This struct represents one particle
//A particle corresponds to both a vertex in the edge system and to a vertex used in rendering triangles
struct Vertex
{
	Vertex()
	{
		position[DIMENSION] = 1;
		velocity[DIMENSION] = 1;
	}
	float position [DIMENSION + 1];	//Position vector in 3d space
	float velocity [DIMENSION + 1];	//Velocity vector in 3d space	
	//double normal [DIMENSION];		//Normal vector in 3d space (this one is a VERTEX AVERAGE used in lighting)
	int triangleCount;				//Number of triangles in which the vertex for this particle is contained (helps calculate normals)
};

//Vertex Struct - position, normal, and color for one vertex in a mesh
struct VertexB
{
	float position[4];
	float normal [4];
	float color[4];
};
