#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include <cglm/cglm.h>
#include <cglm/vec3.h>
#include <cglm/mat4.h> 
#include <cglm/cam.h> 
#include <cglm/affine.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GLuint TextureID;
    int Size[2];
    int Bearing[2];
    unsigned int Advance;
} Character;


 
Character Characters[255];
int resolution_x;
int resolution_y;
const char* vertex_shader_source;
const char* fragment_shader_source;
GLuint VBO, VAO;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); // perform actions based on key presses
void RenderText(GLuint shaderProgram, const char* text, float x, float y, float scale, float color[3]);
const char* load_shader_source(const char* file_path);
GLuint compileShader(GLenum type, const char* source);
GLuint createShaderProgram();
GLFWwindow* setup_opengl(); 
void load_fonts(); // load fonts into global Charaters array



int main(){
    GLFWwindow* window;
    window = setup_opengl();
    GLuint shaderProgram = createShaderProgram();
    load_fonts();

    const char* text = "a suh dud";
    float x = 55.0f;        // X position of the text
    float y = 700.0f;        // Y position of the text
    float scale = 1.0f;     // Scale factor for the text size
    float color[3] = {0.0f, 1.0f, 1.0f};  // RGB color

    while (!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        RenderText(shaderProgram, text, x, y, scale, color);
        RenderText(shaderProgram, "a suh", 600.0f, 55.0f, scale, color);

        glfwPollEvents();
        glfwSwapBuffers(window);
        
    }


    free((void*)vertex_shader_source);
    free((void*)fragment_shader_source);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
      int value;
      if (action == GLFW_PRESS) {
          value = 1;
      } else {
          value = 0;
      }

        switch (key) {
            case GLFW_KEY_ESCAPE:
                exit(0);
                break;
        }
    }
}

void RenderText(GLuint shaderProgram, const char* text, float x, float y, float scale, float color[3]) {
    glUseProgram(shaderProgram); // select which shader program to render with
    mat4 projection;
    glm_ortho(0.0f, resolution_x, 0.0f, resolution_y, -1.0f, 1.0f, projection); // generate a projection matrix related to the resolution of the screen. This is so you can use actual coordinates instead of 0.0f-1.0f in default opengl. In this case the coords are 0-1024 and 0-768
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, (float*)projection); // send this projection to the shader program (in the gpu)so it can be used as it renders

    GLint textColorLocation = glGetUniformLocation(shaderProgram, "textColor"); 
    glUniform3f(textColorLocation, color[0], color[1], color[2]);// send a uniform to shader program for color info

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO); // bind to our one vertex array VAO (global variable)

    for (const char* p = text; *p; p++) { // loop through ever char in the text
        Character ch = Characters[*p];
        //
        //do some math to setup vertices, essentially we are setting up rectangles the rough size of each letter, and spaced apart correctly
        // one character at a time.
        float xpos = x + ch.Bearing[0] * scale;
        float ypos = y - (ch.Size[1] - ch.Bearing[1]) * scale;

        float w = ch.Size[0] * scale;
        float h = ch.Size[1] * scale;
        //printf("letter = %c, xpos: %f, ypos: %f, w: %f, h: %f\n", *p, xpos, ypos, w, h);
        float vertices[6][4] = { // verticies data for one letter
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        }; 

        //printf("letter = %c, textureID = %c\n", *p, ch.TextureID );
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);// bind the current texture to the texture ID of the character we are reading. This is from the array of Character structs we generated in load_fonts()
        glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind to our one buffer VBO (global variable)
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // replace the data in our buffer with the data of this character 
        glBindBuffer(GL_ARRAY_BUFFER, 0);// unbind

        glDrawArrays(GL_TRIANGLES, 0, 6); // draw the letter data in our buffer. It is a rectangle, but has a texture attached that is a bitmap of a letter. This is why if you don't have alpha enabled it will just draw blank rectangles.

        x += (ch.Advance >> 6) * scale; // some special stuff about freetype idk, I believe it's moving us further to the right to prep for next letter
    }

    glBindVertexArray(0); // unbind vertex array
    glBindTexture(GL_TEXTURE_2D, 0); // unbind texture
}

const char* load_shader_source(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", file_path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Could not allocate memory for file: %s\n", file_path);
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

GLuint compileShader(GLenum type, const char* source){
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success){
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
    return shader;
}

GLuint createShaderProgram(){
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success){
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLFWwindow* setup_opengl(){
    GLFWwindow* window;
    resolution_x = 1024;
    resolution_y = 768;
    if (!glfwInit()){exit(-1);}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(resolution_x, resolution_y, "font_rendering", NULL, NULL);
    if (!window){
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        printf("Failed to initialize GLEW\n");
        exit(-1);
    }

    //load shader files into strings
    vertex_shader_source = load_shader_source("vertex_shader.glsl");
    fragment_shader_source = load_shader_source("fragment_shader.glsl");
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // enable alpha channel so they don't render into squares
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // pretty sure this is naughty, but not sure how to change in freetype to align (yet)
    glGenVertexArrays(1, &VAO); // generate 1 vertex array on the GPU
    glGenBuffers(1, &VBO); // generate 1 buffer on the gpu

      
    glBindVertexArray(VAO); // bind to our one and only vertex array
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind to our one and only buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);// fill the buffer with no data (NULL) but choose the size  as 6*4 floats. 6*4 is the size of the vertices data for a character (see renderText function)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0); // create the first attribute pointer with a stride of 4 floats (move 4 floats to get to next attribute). Offset of 0 
    glEnableVertexAttribArray(0); // enable the 0th attribute we defined above
    glBindBuffer(GL_ARRAY_BUFFER, 0);// unbind VBO
    glBindVertexArray(0); // unbind VAO
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); //choose default background color
    glfwSetKeyCallback(window, key_callback); // set callback function (for detecting keypress, we only use ESC in this program)
    return window;
}

void load_fonts(){
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) { // create an init a Freetype Library
        fprintf(stderr, "Could not init FreeType Library\n");
        exit(EXIT_FAILURE);
    }

    FT_Face face;
    if (FT_New_Face(ft, "Montserrat.ttf", 0, &face)) { // load font file into FT_Face object
        fprintf(stderr, "Failed to load font\n");
        exit(EXIT_FAILURE);
    }

    FT_Set_Pixel_Sizes(face, 0, 48); // set font size to 48

    //loop through the first 128 ascii characters 
    for (unsigned char c = 0; c < 128; c++) { 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) { // load ascii char into face
            fprintf(stderr, "Failed to load Glyph\n");
            continue;
        }
        
        GLuint texture;
        glGenTextures(1, &texture); // generate a new texture (empty)
        glBindTexture(GL_TEXTURE_2D, texture); // bind to the newly generated texture
        glTexImage2D( // populate the texture with the data below
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer // actual bitmap/image data being loaded into the GPU
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = { //create a struct with all the data
            texture, // the ID of the texture, used to select correct texture from ones we loaded into the GPU
            {face->glyph->bitmap.width, face->glyph->bitmap.rows}, // size
            {face->glyph->bitmap_left, face->glyph->bitmap_top}, //bearing
            face->glyph->advance.x // advance
        };
        Characters[c] = character; // add it to the global characters array
        //printf("glyh: %c bitmap width: %d, bitmap pitch: %d\n", c, face->glyph->bitmap.width, face->glyph->bitmap.pitch);
        //printf("loaded glyph %c, size: %d - %d, bearing: %d - %d, advance: %ld\n", c, face->glyph->bitmap.width, face->glyph->bitmap.rows,face->glyph->bitmap_left, face->glyph->bitmap_top, face->glyph->advance.x);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}