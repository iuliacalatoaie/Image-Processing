#include<mpi.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} Pixel;

typedef struct {
  char type[5];
  int width;
  int height;
  unsigned char max_val;
  unsigned char** grey;
  Pixel** rgb;
} Image;

char comment[100];

float Smooth[3][3] = {
  {1.0f/9, 1.0f/9, 1.0f/9},
  {1.0f/9, 1.0f/9, 1.0f/9},
  {1.0f/9, 1.0f/9, 1.0f/9}
};

float Blur[3][3] = {
  {1.0f/16, 2.0f/16, 1.0f/16},
  {2.0f/16, 4.0f/16, 2.0f/16},
  {1.0f/16, 2.0f/16, 1.0f/16}
};

float Sharpen[3][3] = {
  {   0.0f, -2.0f/3,    0.0f},
  {-2.0f/3, 11.0f/3, -2.0f/3},
  {   0.0f, -2.0f/3,    0.0f}
};

float Mean[3][3] = {
  {-1.0f, -1.0f,  -1.0f},
  {-1.0f,  9.0f,  -1.0f},
  {-1.0f, -1.0f,  -1.0f}
};

float Emboss[3][3] = {
  {0.f,  1.f, 0.f},
  {0.f,  0.f, 0.f},
  {0.f, -1.f, 0.f}
};

// tipul imaginii va fi vazut ca un int
int get_type(Image image) {
  if (!memcmp(image.type, "P5", 2)) {
    return 1;
  }
  
  return 0;
}

// alocarea memoriei
void matrix_alloc(Image* image) {
  int i;
  int width = image->width + 2;
  int height = image->height + 2;
  
  if (get_type(*image)) {
    image->grey = (unsigned char**)calloc(height, sizeof(unsigned char*));
    for (i = 0; i < height; ++i) {
      image->grey[i] = (unsigned char*)calloc(width, sizeof(unsigned char));
    }
  } else {
    image->rgb = (Pixel**)calloc(height, sizeof(Pixel*));
    
    for (i = 0; i < height; ++i) {
      image->rgb[i] = (Pixel*)calloc(width, sizeof(Pixel));
    }
  }
}

// eliberarea memoriei
void matrix_free(Image* image) {
  int i;
  int height = image->height + 2;
  
  // greuscale
  if (get_type(*image)) {
    for (i = 0; i < height; ++i) {
      free(image->grey[i]);
    }

    free(image->grey);
  } else {
    // rgb
    for (i = 0; i < height; ++i) {
      free(image->rgb[i]);
    }

    free(image->rgb);
  }
}

// citirea datelor de intrare ]ntr-o structura de tip Imagine
Image read_image(char* image_in) {
  FILE *fp;
  int i, j;
  char buffer;
  Image in;

  fp = fopen(image_in, "r");

  fgets(in.type, sizeof(in.type), fp);
  fgets(comment, sizeof(comment), fp);

  fscanf(fp, "%d", &in.width);
  fscanf(fp, "%d", &in.height);
  fscanf(fp, "%hhd", &in.max_val);

  // end of line
  char c = fgetc(fp);
  
  matrix_alloc(&in);

  if (get_type(in)) {
    for (i = 1; i <= in.height; ++i) {
      for (j = 1; j <= in.width; ++j) {
        buffer = fgetc(fp);
        in.grey[i][j] = buffer;
      }
    }
  } else {
    for (i = 1; i <= in.height; ++i) {
      for (j = 1; j <= in.width; ++j) {
        buffer = fgetc(fp);
        in.rgb[i][j].r = buffer;
        
        buffer = fgetc(fp);
        in.rgb[i][j].g = buffer;
        
        buffer = fgetc(fp);
        in.rgb[i][j].b = buffer;
       }
    }
  }
    
  fclose(fp);

  return in;
}

// scrierea datelor de iesire in fisier si dealocarea memoriei
void write_image(Image* in, char* out) {
  FILE *fp;
  int i, j;
  fp = fopen(out, "w");

  fprintf(fp, "%s", in->type);
  fprintf(fp, "%s", comment);
  fprintf(fp, "%d %d\n", in->width, in->height);
  fprintf(fp, "%d\n", in->max_val);
  
  if (get_type(*in)) {
    for (i = 1; i <= in->height; ++i) {
      for (j = 1; j <= in->width; ++j) {
        fputc(in->grey[i][j], fp);
      }
    }
  } else {
    for (i = 1; i <= in->height; ++i) {
      for (j = 1; j <= in->width; ++j) {
        fputc(in->rgb[i][j].r, fp);
        fputc(in->rgb[i][j].g, fp);
        fputc(in->rgb[i][j].b, fp);
      }
    }
  }

  matrix_free(in);

  fclose(fp);
}

// calculez linia de start in functie de rangul procesului
int get_line(int lines, int rank) {
  return (lines * rank + 1);
}

// copierea liniilor dintr-o matrice in alta
void copy_lines(Image* from, Image* to,
                    int start_line, int end_line) {
  int image_type = get_type(*from);
  
  for (int i = start_line; i <= end_line; ++i) {
    for (int j = 1; j <= from->width; ++j) {
      if (image_type) {
        to->grey[i][j] = from->grey[i][j];
      } else {
        to->rgb[i][j].r = from->rgb[i][j].r;
        to->rgb[i][j].g = from->rgb[i][j].g;
        to->rgb[i][j].b = from->rgb[i][j].b;
      }
    }
  } 
}

// copierea atributelor unei imagini
void copy_image(Image* in, Image* out) {
  memcpy(out->type, in->type, 5);
  out->width = in->width;
  out->height = in->height;
  out->max_val = in->max_val;
}

// fiecare filtru va avea o cifra alocata
int get_filter(char* filter) {
  if (!memcmp(filter, "smooth", 6)) {
    return 0;
   }

  if (!memcmp(filter, "blur", 4)) {
    return 1;
  
  }

  if (!memcmp(filter, "sharpen", 7)) {
    return 2;
  }

  if (!memcmp(filter, "mean", 4)) {
    return 3;
  }

  if (!memcmp(filter, "emboss", 6)) {
    return 4;
  }
  
  return -1;
}

// pastrarea in limite a valorii rezultatului
unsigned char bound(float pixel, unsigned char max_val) {
    if (pixel < 0.0f) {
        return 0;
    }
    
    if (pixel >= 255.0f || (unsigned char) pixel >= max_val) {
        return max_val;
    }
       
    return (unsigned char)pixel;
}

// calculul pentru un element din matricea de pixeli
void convolution_product(float filter[3][3], int line, int col,
                         Image in, Image* out) {
  float result;
  if (get_type(in)) {
    result = filter[2][2] * in.grey[line - 1][col - 1] +
             filter[2][1] * in.grey[line - 1][col]     +
             filter[2][0] * in.grey[line - 1][col + 1] +
             
             filter[1][2] * in.grey[line][col - 1]     +
             filter[1][1] * in.grey[line][col]         +
             filter[1][0] * in.grey[line][col + 1]     +
             
             filter[0][2] * in.grey[line + 1][col - 1] +
             filter[0][1] * in.grey[line + 1][col]     +
             filter[0][0] * in.grey[line + 1][col + 1];
                           
    out->grey[line][col] = bound(result, in.max_val);
  } else {
    result = filter[2][2] * in.rgb[line - 1][col - 1].r +
             filter[2][1] * in.rgb[line - 1][col].r     +
             filter[2][0] * in.rgb[line - 1][col + 1].r +
             filter[1][2] * in.rgb[line][col - 1].r     +
             filter[1][1] * in.rgb[line][col].r         +
             filter[1][0] * in.rgb[line][col + 1].r     +
             filter[0][2] * in.rgb[line + 1][col - 1].r +
             filter[0][1] * in.rgb[line + 1][col].r     +
             filter[0][0] * in.rgb[line + 1][col + 1].r;
    out->rgb[line][col].r = bound(result, in.max_val);
                                
    result = filter[2][2] * in.rgb[line - 1][col - 1].g +
             filter[2][1] * in.rgb[line - 1][col].g     +
             filter[2][0] * in.rgb[line - 1][col + 1].g +
             filter[1][2] * in.rgb[line][col - 1].g     +
             filter[1][1] * in.rgb[line][col].g         +
             filter[1][0] * in.rgb[line][col + 1].g     +
             filter[0][2] * in.rgb[line + 1][col - 1].g +
             filter[0][1] * in.rgb[line + 1][col].g     +
             filter[0][0] * in.rgb[line + 1][col + 1].g;
    out->rgb[line][col].g = bound(result, in.max_val);
                          
    result = filter[2][2] * in.rgb[line - 1][col - 1].b +
             filter[2][1] * in.rgb[line - 1][col].b     +
             filter[2][0] * in.rgb[line - 1][col + 1].b +
             filter[1][2] * in.rgb[line][col - 1].b     +
             filter[1][1] * in.rgb[line][col].b         +
             filter[1][0] * in.rgb[line][col + 1].b     +
             filter[0][2] * in.rgb[line + 1][col - 1].b +
             filter[0][1] * in.rgb[line + 1][col].b     +
             filter[0][0] * in.rgb[line + 1][col + 1].b;
    out->rgb[line][col].b = bound(result, in.max_val);
  }
}

// aplicarea filtrului in fuctie de codul matricei
void apply_filter(char* filter, int line, int col, Image in, Image* out) {
  int int_filter = get_filter(filter);
  switch(int_filter) {
    case 0  : convolution_product(Smooth, line, col, in, out);
              break;
    case 1  : convolution_product(Blur, line, col, in, out);
              break;
    case 2  : convolution_product(Sharpen, line, col, in, out);
              break;        
    case 3  : convolution_product(Mean, line, col, in, out);
              break;
    case 4  : convolution_product(Emboss, line, col, in, out);
              break;
    default : break;
  }
}


int main(int argc, char* argv[]) {
  MPI_Status stat;
  Image in, out;
  int rank, nProcesses;
  int start_line, end_line, lines, size, image_type, filters;
  char* filter;

  MPI_Init(&argc, &argv);
  
  MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
  
  if (nProcesses == 1) {
    Image in = read_image(argv[1]);
    Image tmp;
    copy_image(&in, &tmp);
    matrix_alloc(&tmp);
    
    for (int f = 3; f < argc; ++f) {
      copy_lines(&in, &tmp, 1, in.height);

      for (int i = 1; i <= in.height; ++i) {
        for (int j = 1; j <= in.width; ++j) {
          apply_filter(argv[f], i, j, tmp, &in);
        }
      }
    }
    
    matrix_free(&tmp);
    write_image(&in, argv[2]);
  } else {
    // master va fi ultimul proces dupa rang
    const int SRC = nProcesses - 1;
  
    // tipul de date pentru structura de pixel
    const int items = 3;
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR,
                            MPI_UNSIGNED_CHAR};
    MPI_Datatype mpi_rgb_type;
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Pixel, r);
    offsets[1] = offsetof(Pixel, g);
    offsets[2] = offsetof(Pixel, b);

    MPI_Type_create_struct(items, blocklengths, offsets, types, &mpi_rgb_type);
    MPI_Type_commit(&mpi_rgb_type);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
    // rank nProcesses - 1 este master
    if (rank == SRC) {
      /* citesc imaginea, copiez campurile in matricea de prelucrare si
        aloc matricea */
      in = read_image(argv[1]);
      copy_image(&in, &out);
      matrix_alloc(&out);
    
      // numarul de linii trimise fiecarui proces
      lines = in.height / nProcesses;
    
      // tipul imaginii
      image_type = get_type(in);
      filters = argc - 3;
      filter = (char*)calloc(7, sizeof(char));
      memcpy(filter, argv[3], strlen(argv[3]) + 1);
    
      // trimit datele celorlalte procese
      MPI_Bcast(&lines, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(in.type, 5, MPI_CHAR, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.width, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.height, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.max_val, 1, MPI_UNSIGNED_CHAR, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&filters, 1, MPI_INT, SRC, MPI_COMM_WORLD);

      for (int proc = 0; proc < SRC; ++proc) {
        start_line = get_line(lines, proc) - 1;
        end_line = start_line + lines + 1;

        for (int i = start_line; i <= end_line; ++i) {
          if (image_type) {
            MPI_Send(in.grey[i], in.width + 2, MPI_UNSIGNED_CHAR,
                     proc, 0, MPI_COMM_WORLD);
          } else {
            MPI_Send(in.rgb[i], in.width + 2, mpi_rgb_type,
                     proc, 0, MPI_COMM_WORLD);
          }
        }
      }
    
      /* prelucrez restul matricei alocate acestui proces
         respectand regula ca procesele cu rang par trimt si mai apoi primesc,
         iar cele cu rang impar mai intai primesc si apoi trimit pentru a evita
         deadlock */
     start_line = get_line(lines, rank);

      for (int f = 3; f < argc; ++f) {
        MPI_Bcast(argv[f], 7, MPI_CHAR, SRC, MPI_COMM_WORLD);
      
        for (int i = start_line; i <= in.height; ++i) {
          for (int j = 1; j <= in.width; ++j) {
            apply_filter(argv[f], i, j, in, &out);
          }
        }
      
        // daca am mai multe filtre voi copia in in liniile de prelucrat
        if (f < argc - 1) {
          copy_lines(&out, &in, start_line, in.height);
        
          if (rank % 2 == 0) {
            if (image_type) {
              // trimit lui rank -1 linia de bordare superioara rezultata,
              // pentru ca va fi linia lui de bordare inferioara
              MPI_Send(out.grey[start_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD);\
              // astept linia de bordare superioara de la rank - 1,
              // respectiv linia lui de bordare inferioara prelucrata
              MPI_Recv(in.grey[start_line - 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
            } else {
              MPI_Send(out.rgb[start_line], in.width + 2, mpi_rgb_type,
                       rank - 1,  0, MPI_COMM_WORLD);
              MPI_Recv(in.rgb[start_line - 1], in.width + 2, mpi_rgb_type,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
            }
          
          } else {
            if (image_type) {
              MPI_Recv(in.grey[start_line - 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
              MPI_Send(out.grey[start_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD);  
            } else {
              MPI_Recv(in.rgb[start_line - 1], in.width + 2, mpi_rgb_type,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
              MPI_Send(out.rgb[start_line], in.width + 2, mpi_rgb_type,
                     rank - 1,  0, MPI_COMM_WORLD);
            }
          }
        }    
      }
    
      // asteapta primirea liniilor prelucrate de la celelalte procese
      for (int proc = 0; proc < SRC; ++proc) {
        start_line = get_line(lines, proc);
        end_line = start_line + lines - 1;
             
        for (int i = start_line; i <= end_line; ++i) {
          if (image_type) {
            MPI_Recv(out.grey[i], in.width + 2, MPI_UNSIGNED_CHAR, 
                     proc, 0, MPI_COMM_WORLD, &stat);
          } else {
            MPI_Recv(out.rgb[i], in.width + 2, mpi_rgb_type,
                     proc, 0, MPI_COMM_WORLD, &stat);
          }
        }
      }
    
      // eliberez memoria si afisez imaginea
      free(filter);
      matrix_free(&in);
      write_image(&out, argv[2]);
    } else {
     
      filter = (char*)calloc(7, sizeof(char));
      
      // celelalte procese primes datele
      MPI_Bcast(&lines, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(in.type, 5, MPI_CHAR, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.width, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.height, 1, MPI_INT, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&in.max_val, 1, MPI_UNSIGNED_CHAR, SRC, MPI_COMM_WORLD);
      MPI_Bcast(&filters, 1, MPI_INT, SRC, MPI_COMM_WORLD);
    
      // tipul imaginii
      image_type = get_type(in);
      
      // alocarea matricilor si copierea datelor
      matrix_alloc(&in);
      copy_image(&in, &out);
      matrix_alloc(&out);

      // se vor trimite liniile de prelucrat plus cele doua linii de bordare
      start_line = get_line(lines, rank) - 1;
      end_line = start_line + lines + 1;
      
      for (int i = start_line; i <= end_line; ++i) {
        if (image_type) {
          MPI_Recv(in.grey[i], in.width + 2, MPI_UNSIGNED_CHAR,
                   SRC, 0, MPI_COMM_WORLD, &stat);
        } else {
          MPI_Recv(in.rgb[i], in.width + 2, mpi_rgb_type,
                   SRC, 0, MPI_COMM_WORLD, &stat);
        }
     }
    
      // prelucrez liniile fara cele de bordare
      start_line = get_line(lines, rank);
      end_line = start_line + lines - 1;
    
      while (filters > 0) {
        MPI_Bcast(filter, 7, MPI_CHAR, SRC, MPI_COMM_WORLD);
      
        for (int i = start_line; i <= end_line; ++i) {
          for (int j = 1; j <= in.width; ++j) {
            apply_filter(filter, i, j, in, &out);
          }
        }
        --filters;
        
        
        /* in cazul filtrelor multiple suprascriu liniile de interes 
          cu cele prelucrate la un pas anterior */
        if (filters > 0) {
          copy_lines(&out, &in, start_line, end_line);
        
          /* liniile diferite de cele cu rang 0 vor face schimb intre liniile 
            de bordare proprii cu cele ale ambilor vecini
            pentru urmatoarele prelucrari cu aceeasi refula de trimitere
            pe paritate descrisa mai sus*/
          if (rank % 2 == 0 && rank) {
            if (image_type) {
              MPI_Send(out.grey[end_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Send(out.grey[start_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD);
                     
              MPI_Recv(in.grey[end_line + 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD, &stat);
              MPI_Recv(in.grey[start_line - 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
            } else {
              MPI_Send(out.rgb[end_line], in.width + 2, mpi_rgb_type,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Send(out.rgb[start_line], in.width + 2, mpi_rgb_type,
                       rank - 1,  0, MPI_COMM_WORLD);
                     
              MPI_Recv(in.rgb[end_line + 1], in.width + 2, mpi_rgb_type,
                       rank + 1,  0, MPI_COMM_WORLD, &stat);
              MPI_Recv(in.rgb[start_line - 1], in.width + 2, mpi_rgb_type,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
            }
          } else if (rank % 2 != 0) {
            if (image_type) {
              MPI_Recv(in.grey[end_line + 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD, &stat);
              MPI_Recv(in.grey[start_line - 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
                     
              MPI_Send(out.grey[end_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Send(out.grey[start_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank - 1, 0, MPI_COMM_WORLD);
            } else {
              MPI_Recv(in.rgb[end_line + 1], in.width + 2, mpi_rgb_type,
                       rank + 1,  0, MPI_COMM_WORLD, &stat);
              MPI_Recv(in.rgb[start_line - 1], in.width + 2, mpi_rgb_type,
                       rank - 1, 0, MPI_COMM_WORLD, &stat);
                     
              MPI_Send(out.rgb[end_line], in.width + 2, mpi_rgb_type,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Send(out.rgb[start_line], in.width + 2, mpi_rgb_type,
                       rank - 1,  0, MPI_COMM_WORLD);
            }
          } else {
            /* rangul zero nu are vecin superior, rang - 1,
              astfel ca va comunica doar cu 1 */
            if (image_type) {
              MPI_Send(out.grey[end_line], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Recv(in.grey[end_line + 1], in.width + 2, MPI_UNSIGNED_CHAR,
                       rank + 1, 0, MPI_COMM_WORLD, &stat);
            } else {
              MPI_Send(out.rgb[end_line], in.width + 2, mpi_rgb_type,
                       rank + 1, 0, MPI_COMM_WORLD);
              MPI_Recv(in.rgb[end_line + 1], in.width + 2, mpi_rgb_type,
                       rank + 1,  0, MPI_COMM_WORLD, &stat);
            }
          }
        }
      }
    
      // trimit inapoi la ultimul proces liniile prelucrate
      for (int i = start_line; i <= end_line; ++i) {
        if (image_type) {
          MPI_Send(out.grey[i], in.width + 2, MPI_UNSIGNED_CHAR,
                   SRC, 0, MPI_COMM_WORLD);
        } else {
          MPI_Send(out.rgb[i], in.width + 2, mpi_rgb_type,
                 SRC,  0, MPI_COMM_WORLD);
        }
      }  
    
      // eliberarea memoriei
      free(filter);
      matrix_free(&in);
      matrix_free(&out);
    }
  
    MPI_Type_free(&mpi_rgb_type);
  }

  MPI_Finalize();

  return 0;
}
