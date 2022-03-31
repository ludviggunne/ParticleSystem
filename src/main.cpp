#include <iostream>
#include <chrono>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"



// Settings
const glm::vec4 background_color(0.1f);

const bool bounce = true;
const float bounce_damping = .75f;
const float friction = .95f;

const unsigned int pool_size = 6000;
const int emit_batch_size = 120;

const float lifetime = 1.0f;

const glm::vec4 color_begin(0.2f, 0.4f, 1.0f, 1.0f);
const glm::vec4 color_end(1.0f, 1.0f, 1.0f, 0.2f);

const float size_begin = 25.0f;
const float size_end   = 5.0f;

const float size_variation     = 5.0f;
const float velocity_variation = 600.0f;
const float rotation_variation = 14.0f;
const float lifetime_variation = 2.0f;

const float gravity = 2000.0f;

const int window_width  = 800;
const int window_height = 600;



// Typedefs
struct Vertex { glm::vec4 pos; glm::vec4 color; };



// Window properties
GLFWwindow *window;

int width  = window_width;
int height = window_height;



// Open GL
// Errors
void GLclearError() {
	while (glGetError() != GL_NO_ERROR) {}
}

void GLlogError(int line) {

	bool brk = false;
	while (GLenum error = glGetError()) {
		std::cout << "[OpenGL Error] (0x" << std::hex << error << ") at line " << std::dec << line << "\n";
		brk = true;
	}

 	if (brk) __debugbreak();
}

#ifdef _DEBUG
#define GLcall(expr) GLclearError(); expr; GLlogError(__LINE__)
#else
#define GLcall(expr) expr
#endif

// IDs
GLint shader_program;

GLuint vertex_array;
GLuint vertex_buffer;
GLuint index_buffer;

GLint view_location;

// View
glm::mat4 projection;
glm::vec2 view_center;
float view_scale;


// Shaders
#define SHADER_SOURCE(...) #__VA_ARGS__

const char *vertex_source = SHADER_SOURCE(#version 330 core\n
	layout(location = 0) in vec2 v_position;
	layout(location = 1) in vec4 v_color;

	uniform mat4 view_proj;

	out vec4 f_color;

	void main() {
		gl_Position = view_proj * vec4(v_position, 0.0, 1.0);
		f_color = v_color;
	}
);

const char *fragment_source = SHADER_SOURCE(#version 330 core\n
	in vec4 f_color;

	void main() {
		gl_FragColor = f_color;
	}
);



// Utitlities
float randf() {
	return rand() / static_cast<float>(RAND_MAX);
}



// Particle System
unsigned int buffer_size = 0;
unsigned int emit_index = pool_size - 1;

// Buffers
Vertex vertices     [4 * pool_size];
unsigned int indices[6 * pool_size];

// Properties
bool active_states   [pool_size];
					 
glm::vec2 positions  [pool_size];
glm::vec2 velocities [pool_size];
					 
float begin_sizes    [pool_size];
float end_sizes      [pool_size];
float begin_rotations[pool_size];
float rotations      [pool_size];
float lifetimes      [pool_size];



void resizeCallback(GLFWwindow *window, int w, int h) {
	GLcall(glViewport(0, 0, w, h));
	projection = glm::ortho(
		0.0f,
		static_cast<float>(w),
		static_cast<float>(h),
		0.0f,
		-1.0f,
		10.0f
	);
	width = w;
	height = h;
}

glm::mat4 getView() {
	return projection;//glm::translate(glm::scale(projection, glm::vec3(view_scale)), glm::vec3(view_center, 0.0f));
}



int main() {

	// Setup GLFW and window
	glfwInit();

	window = glfwCreateWindow(width, height, "OpenGL Particle System Demo", NULL, NULL);
	
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

	glfwSetWindowSizeCallback(window, &resizeCallback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Init GLAD
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	GLcall(glEnable(GL_BLEND));
	GLcall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	resizeCallback(window, width, height);

	// Setup shaders
	// Compile shaders
	GLcall(GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER));

	GLcall(glShaderSource(vertex_shader, 1, &vertex_source, 0));
	GLcall(glCompileShader(vertex_shader));

	int status;
	char log[512];

	GLcall(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status));
	if (!status) {
		GLcall(glGetShaderInfoLog(vertex_shader, 512, 0, log));
		std::cout << "Vertex shader compile error: " << log << "\n";
	}

	GLcall(GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));

	GLcall(glShaderSource(fragment_shader, 1, &fragment_source, 0));
	GLcall(glCompileShader(fragment_shader));


	GLcall(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status));
	if (!status) {
		GLcall(glGetShaderInfoLog(fragment_shader, 512, 0, log));
		std::cout << "Fragment shader compile error: " << log << "\n";
	}

	// Link shaders
	GLcall(shader_program = glCreateProgram());

	GLcall(glAttachShader(shader_program, vertex_shader));
	GLcall(glAttachShader(shader_program, fragment_shader));
	GLcall(glLinkProgram(shader_program));

	GLcall(glGetProgramiv(shader_program, GL_LINK_STATUS, &status));
	if (!status) {
		GLcall(glGetProgramInfoLog(shader_program, 512, 0, log));
		std::cout << "Shader program linking error: " << log << "\n";
		__debugbreak();
	}

	GLcall(glDeleteShader(vertex_shader));
	GLcall(glDeleteShader(fragment_shader));

	GLcall(glUseProgram(shader_program));
	GLcall(view_location = glGetUniformLocation(shader_program, "view_proj"));



	// Setup indices
	for (int i = 0; i < pool_size; i ++) {
		indices[6 * i + 0] = 4 * i + 0;
		indices[6 * i + 1] = 4 * i + 1;
		indices[6 * i + 2] = 4 * i + 2;
		indices[6 * i + 3] = 4 * i + 1;
		indices[6 * i + 4] = 4 * i + 2;
		indices[6 * i + 5] = 4 * i + 3;
	}


	// Setup Vertex array object
	GLcall(glGenVertexArrays(1, &vertex_array));
	GLcall(glBindVertexArray(vertex_array));

	// Buffers
	GLcall(glGenBuffers(1, &vertex_buffer));
	GLcall(glGenBuffers(1, &index_buffer));

	GLcall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
	GLcall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));

	// Layout
	GLcall(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, pos)));
	GLcall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, color)));
	GLcall(glEnableVertexAttribArray(0));
	GLcall(glEnableVertexAttribArray(1));


	// Init particle system states
	for (int i = 0; i < pool_size; i++)
		active_states[i] = false;


	// Timing
	float delta_time = 1.0f / 60;

	auto prev_mouse = glm::vec2(0.0f);

	// Loop
	while (!glfwWindowShouldClose(window)) {

		// Init delta time
		auto t_begin = std::chrono::high_resolution_clock::now();

		// Clear
		GLcall(glClear(GL_COLOR_BUFFER_BIT));
		GLcall(glClearColor(background_color.r, background_color.g, background_color.b, background_color.a));


		// Get mouse pos
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		auto mouse = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

		// Poll inputs
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{

			for (int i = 0; i < emit_batch_size; i++)
			{
				// Emit particle
				active_states[emit_index] = true;

				begin_sizes[emit_index] = size_begin + size_variation * (randf() - .5f);
				end_sizes[emit_index] = size_end + size_variation * (randf() - .5f);

				rotations[emit_index] = rotation_variation * (randf() - .5f);
				begin_rotations[emit_index] = rotation_variation * (randf() - .5f);

				velocities[emit_index] = velocity_variation * randf() * glm::normalize(glm::vec2(randf() - .5f, randf() - .5f));
				lifetimes[emit_index] = lifetime + lifetime_variation * (randf() - .5f);


				positions[emit_index] = mouse + randf() * (prev_mouse - mouse);


				// Shift emit index
				emit_index = (emit_index + pool_size - 1) % pool_size;

			}

		}
		prev_mouse = mouse;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			for (int i = 0; i < pool_size; i++)
			{
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				glm::vec2 attractor(static_cast<float>(xpos), static_cast<float>(ypos));

				if (active_states[i]) {
					velocities[i] -= 100.0f * (positions[i] - attractor) / glm::length(positions[i] - attractor);
				}
			}
		}
		


		// Update particles and Fill buffers
		buffer_size = 0;
		Vertex *vertex_ptr = vertices;
		for (int i = 0; i < pool_size; i++) {

			if (active_states[i]) {

				// Update particle
				if (lifetime > 0)
				{
					lifetimes[i] -= delta_time;
					if (lifetimes[i] <= 0) {
						active_states[i] = false;
						continue;
					}
				}

				positions[i] += velocities[i] * delta_time;
				velocities[i].y += gravity * delta_time;
				if (positions[i].y > height && bounce)
				{
					velocities[i].x *= friction;
					velocities[i].y *= -bounce_damping * randf();
				}


				// Fill buffers
				auto t = lifetimes[i] / lifetime;
				auto pcolor = color_end + t * (color_begin - color_end);
				auto psize = end_sizes[i] + t * (begin_sizes[i] - end_sizes[i]);
				auto ppos = glm::vec4(positions[i], 0, 0);
				auto rotation = rotations[i] * t + begin_rotations[i];

				const auto axis = glm::vec3(0.0f, 0.0f, 1.0f);

				vertex_ptr->pos = 
					ppos 
					+ glm::rotate(
						glm::mat4(1.0f), 
						rotation, 
						axis) 
					* glm::vec4(-psize, -psize, 0, 0);
				vertex_ptr->color = pcolor;
				vertex_ptr++;

				vertex_ptr->pos = 
					ppos 
					+ glm::rotate(
						glm::mat4(1.0f), 
						rotation, 
						axis) 
					* glm::vec4(psize, -psize, 0, 0);
				vertex_ptr->color = pcolor;
				vertex_ptr++;

				vertex_ptr->pos = 
					ppos 
					+ glm::rotate(
						glm::mat4(1.0f), 
						rotation, 
						axis) 
					* glm::vec4(-psize, psize, 0, 0);
				vertex_ptr->color = pcolor;
				vertex_ptr++;

				vertex_ptr->pos = 
					ppos 
					+ glm::rotate(
						glm::mat4(1.0f), 
						rotation, 
						axis) 
					* glm::vec4(psize, psize, 0, 0);
				vertex_ptr->color = pcolor;
				vertex_ptr++;


				buffer_size++;
			}
		}


		// Set uniform
		GLcall(glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(getView())));

		// Draw
		GLcall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * buffer_size * 4, vertices, GL_STATIC_DRAW));
		GLcall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * buffer_size * 6, indices, GL_STATIC_DRAW));


		GLcall(glDrawElements(GL_TRIANGLES, buffer_size * 6, GL_UNSIGNED_INT, 0));

		glfwSwapBuffers(window);
		glfwPollEvents();


		// Calculate delta time
		auto t_end = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration<float>(t_end - t_begin).count();
	}
	
	GLcall(glDeleteProgram(shader_program));
	glfwTerminate();

	return 0;
}