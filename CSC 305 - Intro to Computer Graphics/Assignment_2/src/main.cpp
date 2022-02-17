// CSC305 Assignment 2
// Matthew Stephenson

// C++ include
#include <iostream>
#include <string>
#include <vector>
//#include <cmath>

// Utilities for the Assignment
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"


// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;



typedef struct {
    MatrixXd R;
    MatrixXd B;
    MatrixXd G;
    MatrixXd A;
    MatrixXd T;  //used to check if objects hit by a ray are covered by a closer object
} scene_output;

typedef struct {
    int width;
    int height;
    enum {ORTHO, PERSP} perspective;
    Vector3d image_origin;
    Vector3d light_position;
    Vector3d camera_origin;
} scene_parameters;

typedef struct {
    Vector3d diffuse_color;
    double specular_exponent;
    Vector3d specular_color;    
    Vector3d ambient_color;
    double ambient;
} shading_parameters;

typedef struct {
    Vector3d center;
    double radius;    	
} sphere_parameters;

typedef struct {
    Vector3d origin;
    Vector3d u;
    Vector3d v;
} pgram_parameters;

typedef struct {
    enum{SPHERE, PGRAM} type;
    sphere_parameters sphere;
    pgram_parameters pgram;
    shading_parameters shading;
} shape;


void raytrace_sphere(scene_output &image, const scene_parameters scene, 
		     const sphere_parameters sphere, const shading_parameters color){
    
    const Vector3d x_displacement(2.0 / scene.width, 0, 0);
    const Vector3d y_displacement(0, -2.0 / scene.height, 0);
   
    const Vector3d ambient_v = color.ambient * color.ambient_color;

    for (unsigned i = 0; i < scene.width; ++i)
    {
        for (unsigned j = 0; j < scene.height; ++j)
        {
	    Vector3d ray_origin;
	    Vector3d ray_direction;
	    
	    if (scene.perspective == scene_parameters::ORTHO) {
		ray_origin = scene.image_origin + double(i) * x_displacement + double(j) * y_displacement;
		ray_direction = Vector3d(0, 0, -1);		
	    }
	    
	    else {
		Vector3d pixel_center = scene.image_origin + double(i) * x_displacement + double(j) * y_displacement;
		ray_origin = scene.camera_origin;
		ray_direction = (pixel_center - ray_origin);
	    }

	    const Vector3d c = (ray_origin - sphere.center);
	    const double disc = std::pow(ray_direction.dot(c), 2.0) - (ray_direction.dot(ray_direction))*(c.dot(c) - std::pow(sphere.radius, 2.0));

            if (disc >= 0) {
		double t = ((-1 * ray_direction).dot(c) - sqrt(disc))/(ray_direction.dot(ray_direction));
		if (t < 0) t = ((-1 * ray_direction).dot(c) + sqrt(disc))/(ray_direction.dot(ray_direction));

		
		//check if the sphere is the closest object incountered
		if ((image.T(i,j) == 0) || (std::abs((t*ray_direction)(2)) < image.T(i,j))) {
		    const Vector3d ray_intersection = ray_origin + t*ray_direction;					  

		    const Vector3d ray_normal = (ray_intersection - sphere.center).normalized();
		    
		    const Vector3d v = (ray_origin - ray_intersection).normalized();
		    const Vector3d light_ray = (scene.light_position - ray_intersection).normalized();
		    const Vector3d phong = (v + light_ray).normalized();

		    const Vector3d diffuse_v = std::max(light_ray.dot(ray_normal), 0.) * color.diffuse_color;
		    const Vector3d specular_v = std::pow(std::max(phong.dot(ray_normal), 0.), color.specular_exponent) * color.specular_color;
               
		    image.R(i, j) = ambient_v(0) + diffuse_v(0) + specular_v(0);
		    image.B(i, j) = ambient_v(1) + diffuse_v(1) + specular_v(1);
		    image.G(i, j) = ambient_v(2) + diffuse_v(2) + specular_v(2);
		    image.T(i, j) = std::abs((t*ray_direction)(2));

		    image.A(i, j) = 1;		    
		}
            }
        }
    }
}

void raytrace_parallelogram(scene_output &image, const scene_parameters scene,
			    const pgram_parameters pgram, const shading_parameters color)
{    
    const Vector3d x_displacement(2.0 / scene.width, 0, 0);
    const Vector3d y_displacement(0, -2.0 / scene.height, 0);

    const Vector3d ambient_v = color.ambient * color.ambient_color;

    for (unsigned i = 0; i < scene.width; ++i)
    {
        for (unsigned j = 0; j < scene.height; ++j)
        {
	    const Vector3d pixel_center = scene.image_origin + double(i) * x_displacement + double(j) * y_displacement;
	    Vector3d ray_origin;
	    Vector3d ray_direction;
	    
	    if (scene.perspective == scene_parameters::ORTHO) {
		ray_origin = pixel_center;
		ray_direction = Vector3d(0,0,-1);		
	    }
	    
	    else {	       	    
		ray_origin = scene.camera_origin;
		ray_direction = pixel_center - ray_origin;
	    }
	     

	    Matrix3d Y;
	    Y << -pgram.u, -pgram.v, ray_direction.normalized();	   
	    Vector3d b = pgram.origin - ray_origin;
	    Vector3d x = Y.inverse()*b;	    	    


	    //only determine ray_intersection if within parallelogram and if it will be the closest object we have encountered
            if ((x(0) <= 1) && (x(0) >= 0) &&
		(x(1) <= 1) && (x(1) >= 0) &&
		((image.T(i,j) == 0) || (x(2) < image.T(i,j))))
            {
		const Vector3d ray_intersection = pgram.origin + x(0) * pgram.u  + x(1) * pgram.v;		  

                Vector3d ray_normal = pgram.v.cross(pgram.u).normalized();

		Vector3d v = (ray_origin - ray_intersection).normalized();
		Vector3d light_ray = (scene.light_position - ray_intersection).normalized();
		Vector3d phong = (v + light_ray).normalized();

		Vector3d diffuse_v = std::max(light_ray.dot(ray_normal), 0.) * color.diffuse_color;
                Vector3d specular_v = std::pow(std::max(phong.dot(ray_normal), 0.), color.specular_exponent) * color.specular_color;
               
                image.R(i, j) = ambient_v(0) + diffuse_v(0) + specular_v(0);
		image.B(i, j) = ambient_v(1) + diffuse_v(1) + specular_v(1);
		image.G(i, j) = ambient_v(2) + diffuse_v(2) + specular_v(2);
		image.T(i, j) = x(2);

		image.A(i, j) = 1;
            }
        }
    }
}


void raytrace(std::string filename, scene_parameters scene, std::vector<shape> objects) {

    std::cout << "Ray tracing to " << filename << std::endl; 

    scene_output image = {
	.R = MatrixXd::Zero(scene.width, scene.height),
	.B = MatrixXd::Zero(scene.width, scene.height),
	.G = MatrixXd::Zero(scene.width, scene.height),
	.A = MatrixXd::Zero(scene.width, scene.height),
	.T = MatrixXd::Zero(scene.width, scene.height)
    };
    
    for (auto & obj: objects) {
	if (obj.type == shape::PGRAM) raytrace_parallelogram(image, scene, obj.pgram, obj.shading);
	else raytrace_sphere(image, scene, obj.sphere, obj.shading);               	
    }    

    write_matrix_to_png(image.R, image.B, image.G, image.A, filename);
	
 }

int main()
{
    int width = 800;
    int height = 800;
    std::vector<shape> objects;
    
    scene_parameters scene = {
	.width = width,
	.height = height,
	.perspective = scene_parameters::ORTHO,
	.image_origin = Vector3d(-1,1,1),
	.light_position = Vector3d(-1,1,1),
	.camera_origin = Vector3d(0,0,3)
    };    
       
    shading_parameters color = {
	.diffuse_color = Vector3d(1, 1, 1),
	.specular_exponent = 100,
	.specular_color = Vector3d(0,0,0),
	.ambient_color = Vector3d(1,1,1),
	.ambient = 0.1
    };

    shading_parameters pink = color;
    pink.diffuse_color = Vector3d(1,0,1);
    pink.specular_color = Vector3d(0,0,1);

    shading_parameters green = color;
    green.diffuse_color = Vector3d(0, 0.6, 0);
    green.specular_color = Vector3d(1, 0, 1);    

    shading_parameters yellow = color;
    yellow.diffuse_color = Vector3d(0.8, 0.8, 0);
    yellow.specular_color = Vector3d(0, 0, 1);

    shading_parameters blue = color;
    blue.diffuse_color = Vector3d(0, 0, 0.4);
    blue.specular_color = Vector3d(1, 1, 0);

        
    // orthographic parallelogram
        
    pgram_parameters pgram = {
	.origin = Vector3d(-0.5, -0.5, 0),
	.u = Vector3d(0, 0.7, -10),
	.v = Vector3d(1, 0.4, 0)
    };

    shape s2 = {
	.type = shape::PGRAM,
	.pgram = pgram,
	.shading = green,
    };

    objects.push_back(s2);
    raytrace("plane_orthographic.png", scene, objects);
    objects.clear();
    
    
    // perpective parallelogram
    
    scene.perspective = scene_parameters::PERSP;
    s2.shading = yellow;
    objects.push_back(s2);
    raytrace("plane_perspective.png", scene, objects);
    objects.clear();



    
    // perpective sphere with pink shading

    sphere_parameters sphere = {
	.center = Vector3d(0,0,0),
	.radius = 0.9,
    };
    
    shape s3 = {
	.type = shape::SPHERE,
	.sphere = sphere,
	.shading = blue
    };

    objects.push_back(s3);
    raytrace("shading.png", scene, objects);
    objects.clear();



    // multiobject scene    

    sphere_parameters sphere_small = {
	.center = Vector3d(0.4, 0, 0),
	.radius = 0.3
    };

         
    shape s4 = {
	.type = shape::SPHERE,
	.sphere = sphere_small,
	.shading = green
    };
    
    objects.push_back(s4);
    
    sphere_small.center = Vector3d(0, 0.2, -1);

    shape s5 = {
	.type = shape::SPHERE,
	.sphere = sphere_small,
	.shading = yellow,
    };

    objects.push_back(s5);

    sphere_parameters sphere_big = {
	.center = Vector3d(2, 0, -4),
	.radius = 2,
    };

    shape s6 = {
	.type = shape::SPHERE,
	.sphere = sphere_big,
	.shading = pink,
    };

    objects.push_back(s6);

    sphere_small.center = Vector3d(-0.4, 0.5, -2);

    shape s7 = {
	.type = shape::SPHERE,
	.sphere = sphere_small,
	.shading = blue,
    };

    objects.push_back(s7);

    sphere_small.center = Vector3d(-0.25, 0.85, -3);

    shape s8 = {
	.type = shape::SPHERE,
	.sphere = sphere_small,
	.shading = yellow,
    };

    objects.push_back(s8);
    
    pgram_parameters floor = {
	.origin = Vector3d(-4, -4, 0),
	.u = Vector3d(0, 4, -10),
	.v = Vector3d(8, 0, 0)
    };

    shape s9 = {
	.type = shape::PGRAM,
	.pgram = floor,
	.shading = color,
    };
    
    objects.push_back(s9);

    pgram_parameters wall = {
	.origin = Vector3d(4, -4, 0),
	.u = Vector3d(-2, 4, -10),
	.v = Vector3d(0, 6, 0),
    };

    shape s10 = {
	.type = shape::PGRAM,
	.pgram = wall,
	.shading = color,
    };
    
    objects.push_back(s10);

    sphere_small.center = Vector3d(0.2, 1.2, -4);

    shape s11 = {
	.type = shape::SPHERE,
	.sphere = sphere_small,
	.shading = green,
    };

    objects.push_back(s11);
    
    raytrace("multiobject.png", scene, objects);            	   
         	 
    return 0;
}
