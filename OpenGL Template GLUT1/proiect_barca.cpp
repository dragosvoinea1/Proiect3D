
//	Biblioteci
#include <windows.h>        //	Utilizarea functiilor de sistem Windows (crearea de ferestre, manipularea fisierelor si directoarelor);
#include <stdlib.h>         //  Biblioteci necesare pentru citirea shaderelor;
#include <stdio.h>
#include <math.h>			//	Biblioteca pentru calcule matematice;
#include <GL/glew.h>        //  Definește prototipurile functiilor OpenGL si constantele necesare pentru programarea OpenGL moderna; 
#include <GL/freeglut.h>    //	Include functii pentru: 
							//	- gestionarea ferestrelor si evenimentelor de tastatura si mouse, 
							//  - desenarea de primitive grafice precum dreptunghiuri, cercuri sau linii, 
							//  - crearea de meniuri si submeniuri;
#include "loadShaders.h"	//	Fisierul care face legatura intre program si shadere;
#include "glm/glm.hpp"		//	Bibloteci utilizate pentru transformari grafice;
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <time.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>

// Dimensiunile ferestrei principale
GLint winWidth = 1200, winHeight = 900;

// Variabile pentru poziția camerei și viteza de mișcare
float cameraPosition[3];
float cameraForward[3];
float cameraUp[3];
float speed;
float angleSpeed;

// Identificatori pentru shaderele utilizate în proiect
GLuint boatShader; // Shader utilizat pentru desenarea bărcii
GLuint waterShader; // Shader utilizat pentru reprezentarea apei

const int maxModels = 2; // Numărul maxim de modele 3D gestionate

// Structura pentru reprezentarea unui vârf al unui obiect 3D
typedef struct Vertex
{
	float position[3]; // Poziția vârfului în spațiu 3D
	float normal[3];   // Vectorul normal utilizat pentru calculul iluminării
	float st[2];       // Coordonatele de textură pentru aplicarea texturii
} Vertex;

// Structura pentru reprezentarea unui model 3D
struct Model
{
public:
	int		numVertices;      // Numărul de vârfuri din model
	struct Vertex* vertices;  // Lista de vârfuri
	int		numTriangles;     // Numărul de triunghiuri care formează modelul
	int		numIndices;       // Numărul de indici
	int* indices;             // Lista de indici utilizată pentru desenare

	GLuint vertexBufferHandle; // Identificator pentru buffer-ul de vârfuri
	GLuint vertexArrayHandle;  // Identificator pentru array-ul de vârfuri

	GLuint shader;             // Shader-ul asociat cu modelul
	float modelPosition[3];    // Poziția modelului în spațiu
	float forward[3];          // Direcția înainte a modelului
	float up[3];               // Vectorul de direcție "în sus" al modelului

	// Constructor pentru model, inițializează shader-ul și poziția implicită
	Model(GLuint shader) : shader(shader)
	{
		setPosition(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);
		up[0] = 0.0f;
		up[1] = 1.0f;
		up[2] = 0.0f;
	}

	// Funcție pentru setarea poziției și direcției modelului
	void setPosition(float x, float y, float z, float forwardx, float forwardy, float forwardz)
	{
		modelPosition[0] = x;
		modelPosition[1] = y;
		modelPosition[2] = z;

		forward[0] = forwardx;
		forward[1] = forwardy;
		forward[2] = forwardz;
	}

	// Funcție pentru calculul matricei de transformare a lumii (world transform)
	glm::mat4 getWorldTransform()
	{
		// Reprezentarea direcțiilor utilizând glm pentru transformări
		glm::vec3 dirforward(forward[0], forward[1], forward[2]);
		glm::vec3 dirup(up[0], up[1], up[2]);
		glm::vec3 dirside = glm::cross(dirup, dirforward);

		// Matricea de rotație pentru orientarea modelului
		glm::mat4 rot = glm::mat4(dirside[0], dirup[0], dirforward[0], 0.0,
			dirside[1], dirup[1], dirforward[1], 0.0,
			dirside[2], dirup[2], dirforward[2], 0.0,
			0.0, 0.0, 0.0, 1.0);

		// Matricea finală include translația, rotația și scalarea modelului
		glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(modelPosition[0], modelPosition[1], modelPosition[2]))
			* rot
			* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 3.0f)); // Scalare
		return M;
	}
};

// Protocoale pentru funcții suplimentare
void	render(Model* model);          // Funcție pentru randarea unui model 3D
bool	parseObjFile(char* filename, Model* model); // Funcție pentru încărcarea unui model dintr-un fișier OBJ
void	calculateNormals(Model* model); // Funcție pentru calculul normelelor pentru iluminare

// Lista de modele utilizate în scenă
Model** models;




void strafe(float direction)
{
	// Funcție pentru deplasarea laterală a camerei (strafe)
	glm::vec3 dirforward(cameraForward[0], cameraForward[1], cameraForward[2]);
	glm::normalize(dirforward); // Normalizează vectorul direcției înainte
	glm::vec3 dirup(cameraUp[0], cameraUp[1], cameraUp[2]);
	glm::normalize(dirup); // Normalizează vectorul direcției în sus
	glm::vec3 dirside = glm::cross(dirup, dirforward); // Calcularea direcției laterale prin produs vectorial

	// Actualizarea poziției camerei pe baza direcției laterale și a vitezei
	cameraPosition[0] += speed * direction * dirside.x;
	cameraPosition[1] += speed * direction * dirside.y;
	cameraPosition[2] += speed * direction * dirside.z;
}

void step(float direction)
{
	// Funcție pentru deplasarea camerei înainte sau înapoi (step)
	glm::vec3 dirforward(cameraForward[0], cameraForward[1], cameraForward[2]);
	glm::normalize(dirforward); // Normalizează vectorul direcției înainte

	// Actualizarea poziției camerei pe baza direcției înainte și a vitezei
	cameraPosition[0] += speed * direction * dirforward.x;
	cameraPosition[1] += speed * direction * dirforward.y;
	cameraPosition[2] += speed * direction * dirforward.z;
}

glm::mat3 rotationVectorAngle(float ux, float uy, float uz, float angle)
{
	// Funcție pentru generarea unei matrice de rotație în jurul unui vector dat (ux, uy, uz) cu un anumit unghi
	float ct = cos(angle); // Cosinusul unghiului
	float st = sin(angle); // Sinusul unghiului
	float oct = 1.0f - ct; // Complementul cosinusului

	// Matricea de rotație este generată folosind formula Rodrigues
	glm::mat3 rot = glm::mat3(ct + ux * ux * oct, ux * uy * oct - uz * st, ux * uz * oct + uy * st,
		uy * ux * oct + uz * st, ct + uy * uy * oct, uy * uz * oct - ux * st,
		uz * ux * oct - uy * st, uz * uy * oct + ux * st, ct + uz * uz * oct);
	return rot;
}

void turnside(float direction)
{
	// Funcție pentru rotația camerei pe orizontală (stânga/dreapta)
	glm::vec3 dirforward(cameraForward[0], cameraForward[1], cameraForward[2]);
	glm::normalize(dirforward); // Normalizează vectorul direcției înainte
	glm::vec3 dirup(cameraUp[0], cameraUp[1], cameraUp[2]);
	glm::normalize(dirup); // Normalizează vectorul direcției în sus

	// Calcularea unghiului de rotație
	float angle = direction * angleSpeed;
	glm::mat3 R = rotationVectorAngle(dirup.x, dirup.y, dirup.z, angle); // Matricea de rotație în jurul direcției în sus

	dirforward = R * dirforward; // Aplică rotația asupra direcției înainte

	// Actualizarea direcției camerei
	cameraForward[0] = dirforward.x;
	cameraForward[1] = dirforward.y;
	cameraForward[2] = dirforward.z;
}

void turnup(float direction)
{
	// Funcție pentru rotația camerei pe verticală (sus/jos)
	glm::vec3 dirforward(cameraForward[0], cameraForward[1], cameraForward[2]);
	glm::normalize(dirforward); // Normalizează vectorul direcției înainte
	glm::vec3 dirup(cameraUp[0], cameraUp[1], cameraUp[2]);
	glm::normalize(dirup); // Normalizează vectorul direcției în sus
	glm::vec3 dirside = glm::cross(dirup, dirforward); // Calcularea direcției laterale

	// Calcularea unghiului de rotație
	float angle = direction * angleSpeed;
	glm::mat3 R = rotationVectorAngle(dirside.x, dirside.y, dirside.z, angle); // Matricea de rotație în jurul direcției laterale

	dirforward = R * dirforward; // Aplică rotația asupra direcției înainte
	dirup = R * dirup;           // Aplică rotația asupra direcției în sus

	// Actualizarea direcției camerei
	cameraForward[0] = dirforward.x;
	cameraForward[1] = dirforward.y;
	cameraForward[2] = dirforward.z;
	cameraUp[0] = dirup.x;
	cameraUp[1] = dirup.y;
	cameraUp[2] = dirup.z;
}

void ProcessNormalKeys(unsigned char key, int x, int y)
{
	// Funcție pentru gestionarea intrărilor de la tastatură pentru taste normale
	switch (key) {
	case 'a': // Deplasare laterală spre stânga
		strafe(-1.0);
		break;
	case 'd': // Deplasare laterală spre dreapta
		strafe(1.0);
		break;
	case 'w': // Deplasare înainte
		step(-1.0);
		break;
	case 's': // Deplasare înapoi
		step(1.0);
		break;
	}

	// Închidere aplicație dacă se apasă tasta Escape
	if (key == 27)
		exit(0);
}

void ProcessSpecialKeys(int key, int xx, int yy)
{
	// Funcție pentru gestionarea intrărilor de la tastatură pentru taste speciale
	switch (key)
	{
	case GLUT_KEY_LEFT: // Rotație spre stânga
		turnside(-1.0);
		break;
	case GLUT_KEY_RIGHT: // Rotație spre dreapta
		turnside(1.0);
		break;
	case GLUT_KEY_UP: // Rotație în sus
		turnup(-1.0);
		break;
	case GLUT_KEY_DOWN: // Rotație în jos
		turnup(1.0);
		break;
	}
}


//	Schimba inaltimea/latimea scenei in functie de modificarile facute de utilizator ferestrei (redimensionari);
void ReshapeFunction(GLint newWidth, GLint newHeight)
{
	// Actualizează viewport-ul pentru a se potrivi cu noile dimensiuni ale ferestrei
	glViewport(0, 0, newWidth, newHeight);
	winWidth = newWidth;  // Stochează lățimea ferestrei
	winHeight = newHeight; // Stochează înălțimea ferestrei
}

//  Functia de eliberare a resurselor alocate de program;
void Cleanup(void)
{
}

//  Setarea parametrilor necesari pentru fereastra de vizualizare;
void Initialize(void)
{
	// Setarea poziției inițiale a camerei
	cameraPosition[0] = 0.0f;
	cameraPosition[1] = 3.0f;
	cameraPosition[2] = -6.0f;

	// Setarea direcției inițiale a camerei
	cameraForward[0] = 0.0f;
	cameraForward[1] = 0.0f;
	cameraForward[2] = -1.0f;

	// Setarea vectorului "sus" pentru cameră
	cameraUp[0] = 0.0f;
	cameraUp[1] = 1.0f;
	cameraUp[2] = 0.0f;

	speed = 0.2f;        // Viteza de mișcare
	angleSpeed = 0.3f;   // Viteza de rotație

	// Setarea culorii de fundal a ferestrei (negru)
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Inițializarea generatorului de numere aleatoare
	srand(time(NULL));

	// Încărcarea shaderelor pentru barcă și apă
	boatShader = LoadShaders("float.vert", "float.frag");
	waterShader = LoadShaders("plane.vert", "plane.frag");

	// Crearea unui vector pentru modele
	models = new Model * [maxModels];

	// Crearea și configurarea modelului planului de apă
	Model* waterPlaneModel = new Model(waterShader);
	parseObjFile((char*)"plane.obj", waterPlaneModel); // Încărcarea modelului din fișier OBJ
	calculateNormals(waterPlaneModel);                // Calcularea normelelor pentru iluminare

	// Configurarea bufferelor pentru modelul planului de apă
	glGenBuffers(1, &waterPlaneModel->vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, waterPlaneModel->vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * waterPlaneModel->numVertices, waterPlaneModel->vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &waterPlaneModel->vertexArrayHandle);
	glBindVertexArray(waterPlaneModel->vertexArrayHandle);
	glEnableVertexAttribArray(0 /* index */); // Poziție
	glVertexAttribPointer(0 /* index */, 3 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
	glEnableVertexAttribArray(1 /* index */); // Normale
	glVertexAttribPointer(1 /* index */, 3 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2 /* index */); // Textură
	glVertexAttribPointer(2 /* index */, 2 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, st));

	// Crearea și configurarea modelului bărcii
	Model* boatModel = new Model(boatShader);
	parseObjFile((char*)"boat.obj", boatModel); // Încărcarea modelului din fișier OBJ
	calculateNormals(boatModel);               // Calcularea normelelor pentru iluminare

	// Configurarea bufferelor pentru modelul bărcii
	glGenBuffers(1, &boatModel->vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, boatModel->vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * boatModel->numVertices, boatModel->vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &boatModel->vertexArrayHandle);
	glBindVertexArray(boatModel->vertexArrayHandle);
	glEnableVertexAttribArray(0 /* index */); // Poziție
	glVertexAttribPointer(0 /* index */, 3 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
	glEnableVertexAttribArray(1 /* index */); // Normale
	glVertexAttribPointer(1 /* index */, 3 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2 /* index */); // Textură
	glVertexAttribPointer(2 /* index */, 2 /* components */, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, st));

	// Adăugarea modelelor în scenă
	models[0] = waterPlaneModel;
	models[0]->setPosition(0.0f, 0.0f, 0.0f, 0.0, 0.0, 1.0f); // Poziția planului de apă

	models[1] = boatModel;
	models[1]->setPosition(0.0f, -4.0f, 0.0f, 0.0, 0.0, 1.0f); // Poziția bărcii
}

//	Functia de desenare a graficii pe ecran;
void RenderFunction(void)
{
	// Curățarea bufferelor de culoare și profunzime
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST); // Activarea testului de adâncime

	// Configurarea matricelor de vizualizare, proiecție și umbrire
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 100.0f); // Matrice de proiecție perspectivă
	glm::mat4 shadow = glm::mat4(1.0f); // Matrice de umbrire (implicită)

	// Calcularea transformării camerei
	glm::vec3 dirforward(cameraForward[0], cameraForward[1], cameraForward[2]);
	glm::normalize(dirforward);
	glm::vec3 dirup(cameraUp[0], cameraUp[1], cameraUp[2]);
	glm::normalize(dirup);
	glm::vec3 dirside = glm::cross(dirup, dirforward);

	glm::mat4 rot = glm::mat4(dirside[0], dirside[1], dirside[2], 0.0,
		dirup[0], dirup[1], dirup[2], 0.0,
		dirforward[0], dirforward[1], dirforward[2], 0.0,
		0.0, 0.0, 0.0, 1.0);
	glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(cameraPosition[0], cameraPosition[1], cameraPosition[2])) * rot;
	view = glm::inverse(M);

	// Randarea fiecărui model din scenă
	for (int i = 0; i < maxModels; i++)
	{
		Model* model = models[i];

		static float pauto = 0.0f;
		float ranNum = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

		glm::mat4 world = model->getWorldTransform(); // Matricea transformării globale
		GLuint shader = model->shader;

		glUseProgram(shader); // Activarea shader-ului curent

		// Transmiterea matricelor de transformare către shader
		glUniformMatrix4fv(glGetUniformLocation(shader, (char*)"WorldMatrix"), 1, GL_FALSE, &world[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader, (char*)"ViewMatrix"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader, (char*)"ProjectionMatrix"), 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader, (char*)"ShadowMatrix"), 1, GL_FALSE, &shadow[0][0]);

		// Configurarea parametrilor de iluminare
		glUniform1i(glGetUniformLocation(shader, (char*)"ShadowMap"), 0);
		float lx = 0.0;
		float ly = -3.5;
		float lz = 0.0;
		glUniform4f(glGetUniformLocation(shader, (char*)"LightPosition"), lx, ly, lz, 1.0f);

		// Parametri dinamici pentru animație
		glUniform1f(glGetUniformLocation(shader, (char*)"pauto"), pauto);
		pauto += 0.05f;
		glUniform1f(glGetUniformLocation(shader, (char*)"ranNum"), ranNum);

		// Apelarea funcției de randare pentru modelul curent
		render(model);
	}

	// Schimbarea bufferelor pentru a afișa rezultatul pe ecran
	glutSwapBuffers();
	glFlush();
}



//	Punctul de intrare in program, se ruleaza rutina OpenGL;
int main(int argc, char* argv[])
{
	//  Se initializeaza GLUT si contextul OpenGL si se configureaza fereastra si modul de afisare;

	glutInit(&argc, argv);

	glutInitContextVersion(3, 2); // Set OpenGL 3.2
	glutInitContextProfile(GLUT_CORE_PROFILE); // Use the core profile (not compatibility)


	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);		//	Se folosesc 2 buffere pentru desen (unul pentru afisare si unul pentru randare => animatii cursive) si culori RGB + 1 buffer pentru adancime;
	glutInitWindowSize(winWidth, winHeight);						//  Dimensiunea ferestrei;
	glutInitWindowPosition(100, 100);								//  Pozitia initiala a ferestrei;
	glutCreateWindow("Proiect Barca 3D - OpenGL <<nou>>");		//	Creeaza fereastra de vizualizare, indicand numele acesteia;

	//	Se initializeaza GLEW si se verifica suportul de extensii OpenGL modern disponibile pe sistemul gazda;
	//  Trebuie initializat inainte de desenare;

	glewInit();

	Initialize();							//  Setarea parametrilor necesari pentru fereastra de vizualizare; 
	glutReshapeFunc(ReshapeFunction);		//	Schimba inaltimea/latimea scenei in functie de modificarile facute de utilizator ferestrei (redimensionari);
	glutDisplayFunc(RenderFunction);		//  Desenarea scenei in fereastra;
	glutIdleFunc(RenderFunction);			//	Asigura rularea continua a randarii;
	glutKeyboardFunc(ProcessNormalKeys);	//	Functii ce proceseaza inputul de la tastatura utilizatorului;
	glutSpecialFunc(ProcessSpecialKeys);
	glutCloseFunc(Cleanup);					//  Eliberarea resurselor alocate de program;



	glutMainLoop();

	return 0;
}


void render(Model* model)
{
	// Functie pentru randarea unui model 3D
	glBindVertexArray(model->vertexArrayHandle); // Leaga array-ul de varfuri al modelului

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Seteaza modul de desenare (umplere completa a poligoanelor)
	glDrawElements(GL_TRIANGLES, model->numIndices, GL_UNSIGNED_INT, model->indices); // Desenarea elementelor modelului
}


typedef struct Triple
{
	// Structura pentru reprezentarea unei coordonate in spatiul 3D
	float x; // Coordonata pe axa X
	float y; // Coordonata pe axa Y
	float z; // Coordonata pe axa Z
} Triple;

typedef struct Face
{
	// Structura pentru reprezentarea unei fete a unui model (triunghi)
	int triangle[3];   // Indicii varfurilor triunghiului
	int texcoord[3];   // Indicii coordonatelor de textura
	int normcoord[3];  // Indicii vectorilor normali
} Face;


bool parseObjFile(char* filename, Model* model)
{   // Functie pentru parsarea unui fisier OBJ si incarcare intr-un model 3D
	std::vector<Triple*> vertexCoordinates; // Lista coordonatelor de varfuri
	std::vector<Triple*> textureCoordinates; // Lista coordonatelor de textura
	std::vector<Triple*> normalCoordinates; // Lista vectorilor normali

	std::vector<Face*> faces; // Lista fetelor (triunghiuri)

	std::string modelfullpath = filename; // Numele fisierului modelului

	std::ifstream fl(modelfullpath); // Deschide fisierul
	if (fl.is_open()) // Verifica daca fisierul este deschis
	{
		std::string line;
		while (fl.good()) // Citeste fiecare linie din fisier
		{
			getline(fl, line);

			if (line.length() > 0)
			{
				switch (line[0])
				{
				case '#': // Linie de comentariu
					break;
				case 'g':  // Grup de obiecte
					break;
				case 'v': // Coordonate de varf, normale sau textura
				{
					std::stringstream sline(line);
					char c;

					if (line[1] == 't') // Coordonate de textura
					{
						Triple* p = new Triple();
						sline >> c >> c >> (*p).x >> (*p).y >> (*p).z;
						textureCoordinates.push_back(p);
					}
					else if (line[1] == 'n')  // Vectori normali
					{
						Triple* norm = new Triple();
						sline >> c >> c >> (*norm).x >> (*norm).y >> (*norm).z;

						normalCoordinates.push_back(norm);
					}
					else // Coordonate de varf
					{
						Triple* coord = new Triple();
						sline >> c >> (*coord).x >> (*coord).y >> (*coord).z;

						vertexCoordinates.push_back(coord);
					}
				}
				break;
				case 'f': // Definirea unei fete (triunghi)
				{
					std::stringstream sline(line);
					char c;
					sline >> c;
					Face* f = new Face();
					int count = 0;
					while (true)
					{
						int vertex;
						if (!sline.good())
							break;
						sline >> vertex;
						if (sline.fail()) {
							sline.clear();
							break;
						}
						if (count >= 3) // Triangulatie pentru poligoane complexe
						{
							faces.push_back(f);
							Face* nf = new Face();
							(*nf).triangle[0] = (*f).triangle[0];
							(*nf).triangle[1] = (*f).triangle[2];
							(*nf).texcoord[0] = (*f).texcoord[0];
							(*nf).texcoord[1] = (*f).texcoord[2];
							(*nf).normcoord[0] = (*f).normcoord[0];
							(*nf).normcoord[1] = (*f).normcoord[2];
							f = nf;
							count--;
						}
						(*f).triangle[count] = vertex - 1; // Indicii incep de la 0

						sline.get(c);
						if (c == '/')
						{
							if (sline.peek() == '/')
							{
								sline.get(c);
								int normcoord;
								sline >> normcoord;
								(*f).normcoord[count] = normcoord - 1; // Vector normal
								(*f).texcoord[count] = 0; // Coordonate de textura implicite
							}
							else
							{
								int texture;
								sline >> texture;
								(*f).texcoord[count] = texture - 1; // Coordonate de textura

								if (sline.peek() == '/')
								{
									sline.get(c);
									int normcoord;
									sline >> normcoord;
									(*f).normcoord[count] = normcoord - 1; // Vector normal
								}
							}
						}
						count++;
					}
					faces.push_back(f);
				}
				break;
				}
			}
		}
		fl.close(); // Inchiderea fisierului
	}
	else
	{


		// Eliberarea resurselor in cazul unui esec
		while (!vertexCoordinates.empty())
		{
			Triple* vp = vertexCoordinates.back();
			delete vp;
			faces.pop_back();
		}

		while (!textureCoordinates.empty())
		{
			Triple* tp = textureCoordinates.back();
			delete tp;
			faces.pop_back();
		}

		while (!normalCoordinates.empty())
		{
			Triple* np = normalCoordinates.back();
			delete np;
			faces.pop_back();
		}

		while (!faces.empty())
		{
			Face* fp = faces.back();
			delete fp;
			faces.pop_back();
		}

		return false; // Eroare la deschiderea fisierului
	}


	// Structura pentru reprezentarea unui vârf al unui triunghi, inclusiv indicele său de poziție, textură și normală
	typedef struct _VVV {
		int v; // Indicele coordonatei vârfului
		int t; // Indicele coordonatei de textură
		int n; // Indicele coordonatei normale
	} VVV;

	// Vector temporar pentru stocarea vârfurilor unice înainte de transferul în model
	std::vector <VVV> vertexTempVector;

	// Map pentru asigurarea unicității vârfurilor în bufferul final de vârfuri
	typedef std::map <std::tuple<int, int, int>, int> VertexMapType;
	VertexMapType myVertexMap;

	// Calcularea numărului de triunghiuri și inițializarea bufferelor pentru indici și vârfuri
	model->numTriangles = faces.size(); // Numărul de triunghiuri
	model->numIndices = 3 * model->numTriangles; // Numărul total de indici
	model->indices = new int[model->numIndices]; // Alocarea bufferului pentru indici
	int indiceCnt = 0; // Contor pentru indici

	int* newindices = new int[model->numIndices]; // Buffer temporar pentru indici
	int newindiceCnt = 0; // Contor pentru indici temporari
	int vCount = 0; // Contor pentru vârfuri unice

	// Iterare prin toate fețele modelului pentru a popula bufferul de indici și lista de vârfuri unice
	for (size_t findex = 0; findex < faces.size(); findex++)
	{
		for (int tri = 0; tri < 3; tri++)
		{
			VVV v;

			// Extrage indicii pentru coordonate, textură și normale din față
			v.v = faces[findex]->triangle[tri];
			v.t = faces[findex]->texcoord[tri];
			v.n = faces[findex]->normcoord[tri];


			// Verifică dacă vârful este deja în map folosind tuple (v, t, n)
			typedef std::tuple<int, int, int> TupleType;
			TupleType myTuple = std::make_tuple(v.v, v.t, v.n);

			VertexMapType::iterator it2 = myVertexMap.find(myTuple);
			if (it2 != myVertexMap.end())
			{
				// Dacă vârful există deja, se reutilizează indicele existent
				model->indices[indiceCnt++] = (*it2).second;
			}
			else
			{	// Dacă vârful este nou, se adaugă în vectorul temporar și în map
				vertexTempVector.push_back(v);
				model->indices[indiceCnt++] = vCount; // Se adaugă indicele nou
				myVertexMap[myTuple] = vCount; // Se map-ează tuple-ul la noul indice
				vCount++; // Incrementarea contorului de vârfuri
			}

		}
	}


	// Calcularea numărului de vârfuri unice și alocarea bufferului pentru vârfuri
	model->numVertices = vertexTempVector.size();
	model->vertices = new Vertex[model->numVertices];


	// Popularea bufferului de vârfuri cu datele unice (poziție, textură, normale)
	for (std::vector<int>::size_type index = 0; index < vertexTempVector.size(); index++)
	{	// Poziția vârfului
		model->vertices[index].position[0] = vertexCoordinates[vertexTempVector[index].v]->x;
		model->vertices[index].position[1] = vertexCoordinates[vertexTempVector[index].v]->y;
		model->vertices[index].position[2] = vertexCoordinates[vertexTempVector[index].v]->z;

		// Coordonatele de textură, dacă există
		if (textureCoordinates.size() > 0)
		{
			model->vertices[index].st[0] = textureCoordinates[vertexTempVector[index].t]->x;
			model->vertices[index].st[1] = textureCoordinates[vertexTempVector[index].t]->y;
		}
		else
		{	// Implicit, coordonatele de textură sunt 0
			model->vertices[index].st[0] = 0.0;
			model->vertices[index].st[1] = 0.0;
		}

		// Vectorii normali, dacă există
		if (normalCoordinates.size() > 0)
		{
			model->vertices[index].normal[0] = normalCoordinates[vertexTempVector[index].n]->x;
			model->vertices[index].normal[1] = normalCoordinates[vertexTempVector[index].n]->y;
			model->vertices[index].normal[2] = normalCoordinates[vertexTempVector[index].n]->z;
		}
		else
		{	// Implicit, vectorii normali sunt 0
			model->vertices[index].normal[0] = 0.0;
			model->vertices[index].normal[1] = 0.0;
			model->vertices[index].normal[2] = 0.0;
		}
	}

	// Eliberarea memoriei alocate pentru datele temporare
	while (!vertexCoordinates.empty())
	{
		Triple* vp = vertexCoordinates.back();
		delete vp;
		vertexCoordinates.pop_back();
	}

	while (!textureCoordinates.empty())
	{
		Triple* tp = textureCoordinates.back();
		delete tp;
		textureCoordinates.pop_back();
	}

	while (!normalCoordinates.empty())
	{
		Triple* np = normalCoordinates.back();
		delete np;
		normalCoordinates.pop_back();
	}

	while (!faces.empty())
	{
		Face* fp = faces.back();
		delete fp;
		faces.pop_back();
	}
	// Returnează true pentru a indica succesul încărcării modelului
	return true;
}


void calculateNormals(Model* model)
{
	// Inițializarea tuturor normalelor vârfurilor cu 0
	for (int i = 0; i < model->numVertices; i++) {
		model->vertices[i].normal[0] = 0.0f;
		model->vertices[i].normal[1] = 0.0f;
		model->vertices[i].normal[2] = 0.0f;
	}

	// Calcularea normalelor pentru fiecare triunghi din model
	for (int i = 0; i < model->numTriangles; i++) {
		// Obține coordonatele vârfurilor triunghiului
		float* A = (float*)&(model->vertices[model->indices[3 * i + 0]].position); // Vârf A
		float* B = (float*)&(model->vertices[model->indices[3 * i + 1]].position); // Vârf B
		float* C = (float*)&(model->vertices[model->indices[3 * i + 2]].position); // Vârf C

		// Calculează laturile triunghiului
		glm::vec3 side1 = glm::vec3(C[0], C[1], C[2]) - glm::vec3(A[0], A[1], A[2]);
		glm::vec3 side2 = glm::vec3(B[0], B[1], B[2]) - glm::vec3(A[0], A[1], A[2]);

		// Calculează normalul triunghiului utilizând produsul vectorial (cross product)
		glm::vec3 normal = glm::cross(side2, side1);

		// Adaugă normalul triunghiului la fiecare dintre cele trei vârfuri ale sale
		for (int v = 0; v < 3; v++) {
			model->vertices[model->indices[3 * i + v]].normal[0] += normal.x;
			model->vertices[model->indices[3 * i + v]].normal[1] += normal.y;
			model->vertices[model->indices[3 * i + v]].normal[2] += normal.z;
		}
	}

	// Normalizează normalele pentru fiecare vârf
	for (int i = 0; i < model->numVertices; i++) {
		float* N = (float*)&(model->vertices[i].normal);

		// Normalizează vectorul normal utilizând glm
		glm::vec3 normal = glm::normalize(glm::vec3(N[0], N[1], N[2]));

		// Salvează normalul normalizat în model
		N[0] = normal.x;
		N[1] = normal.y;
		N[2] = normal.z;
	}
}

