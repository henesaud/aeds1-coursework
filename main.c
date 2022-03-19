#include <stdio.h>
#include <allegro5/allegro.h>
#include <time.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <math.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

typedef struct bago
{
   float posicao_x;
   float posicao_y;
   float raio;
   float posicao_addx;
   float posicao_addy;
   int red, gre, blu;
   float incremento;
} bago;

float dist(float x1, float x2, float y1, float y2)
{
   return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

typedef struct Background
{
   float x;
   float y;
   float velX;
   float velY;
   int dirX;
   int dirY;
   int largura;
   int altura;
   ALLEGRO_BITMAP *image;
} Background;

int raio = 7; // RAIO MAXIMO DAS BOLAS INIMIGAS
const float FPS = 60;
const int L = 1400;
const int A = 800;
const int V_MAXIMA_BAGOS = 3; // VELOCIDADE MAXIMA DAS BOLAS INIMIGAS

float calcular_distancia_bolas(bago a, bago *b)
{
   return sqrt(pow(abs((a.posicao_x - b->posicao_x)), 2) + pow(abs((a.posicao_y - b->posicao_y)), 2));
}

float calcular_distancia_bolas_2(bago *a, bago *b)
{
   return sqrt(pow(abs((a->posicao_x - b->posicao_x)), 2) + pow(abs((a->posicao_y - b->posicao_y)), 2));
}

bago *cria_bago_aleatorio(int v)
{
   bago *retorno = (bago *)malloc(sizeof(bago));
   if (v != 0)
      retorno->raio = 4 + rand() % raio;
   if (v == 0)
      retorno->raio = 0;
   retorno->posicao_x = (retorno->raio) + rand() % (L - raio * 2);
   retorno->posicao_y = (retorno->raio) + rand() % (A - raio * 2);
   retorno->posicao_addx = (0.5 + rand() % V_MAXIMA_BAGOS) / -4 + (rand() % 3);
   retorno->posicao_addy = (0.5 + rand() % V_MAXIMA_BAGOS) / -4 + (rand() % 3);
   retorno->red = 230;
   retorno->gre = 230;
   retorno->blu = 30 + rand() % 230;
   return retorno;
}

int main(int argc, char **argv)
{
   bool resultado = false;
   srand(time(NULL));
   int N_BOLAS_SHOWED = 80;
   int h = 0;
   int i;
   int raio_principal = 25;
   float score = 0;
   float score_final;
   int inimiga = N_BOLAS_SHOWED - 1;
   int impulso_expelidas = 1.0;

   int aux_bola_aleatoria = rand() % 3;

   ALLEGRO_DISPLAY *tela = NULL;
   ALLEGRO_EVENT_QUEUE *fila_eventos = NULL;
   ALLEGRO_TIMER *temporizador = NULL;
   al_init_primitives_addon();
   al_init_font_addon();
   al_init_ttf_addon();

   // ABRE ARQUIVO COM SCORE
   FILE *log = fopen("score_log.txt", "r+");

   // DECLARA AS BOLAS E O FUNDO
   bago **bola = (bago **)malloc(1000 * sizeof(bago *));

   for (i = 0; i < N_BOLAS_SHOWED; i++)
   {
      bola[i] = cria_bago_aleatorio(1);
   }

   bola[inimiga]->raio = raio_principal + 20;
   bola[inimiga]->red = 0;
   bola[inimiga]->gre = 40;
   bola[inimiga]->blu = 255;
   bola[inimiga]->posicao_x = 80;
   bola[inimiga]->posicao_y = 80;

   bool redesenhar = true;
   Background BG;
   Background MG;

   // DECLARA A BOLA PRINCIPAL
   bago principal;

   principal.raio = raio_principal;
   principal.posicao_x = L / 2;
   principal.posicao_y = A / 2;
   principal.posicao_addx = 0;
   principal.posicao_addy = -principal.posicao_addx;
   principal.red = 255;
   principal.gre = 0;
   principal.blu = 0;
   principal.incremento = 1;

   // ROTINA DE INICIALIZAÇÃO
   if (!al_init())
   {
      fprintf(stderr, "Erro ao carregar allegro!!\n");
      return -1;
   }

   temporizador = al_create_timer(1.0 / FPS);
   if (!temporizador)
   {
      fprintf(stderr, "Erro ao carregar temporizador!\n");
      return -1;
   }

   tela = al_create_display(L, A);
   if (!tela)
   {
      fprintf(stderr, "Erro ao criar display!\n");
      al_destroy_timer(temporizador);
      return -1;
   }
   if (!al_install_mouse())
      fprintf(stderr, "Erro ao carregar mouse!\n");

   if (!al_install_audio())
   {
      fprintf(stderr, "Falha ao iniciar audio!\n");
      return -1;
   }

   if (!al_reserve_samples(1))
   {
      fprintf(stderr, "Falha ao reservar sample!\n");
      return -1;
   }

   // INICIA SONS

   al_set_target_bitmap(al_get_backbuffer(tela));

   al_clear_to_color(al_map_rgb(255, 255, 255));

   // CRIA FILA DE EVENTOS
   fila_eventos = al_create_event_queue();
   al_register_event_source(fila_eventos, al_get_timer_event_source(temporizador));
   if (!fila_eventos)
   {
      fprintf(stderr, "Erro ao carregar fila eventos!\n");
      al_destroy_display(tela);
      al_destroy_timer(temporizador);
      return -1;
   }
   // ADDONS//
   al_init_image_addon();
   al_init_acodec_addon();

   // ABRE E SETA IMAGENS E SONS//
   ALLEGRO_BITMAP *bgImage = al_load_bitmap("starBG.png");
   ALLEGRO_BITMAP *mgImage = al_load_bitmap("starMG.png");
   ALLEGRO_BITMAP *sc = al_load_bitmap("score.png");
   ALLEGRO_SAMPLE *inicio = al_load_sample("inicio.wav");
   ALLEGRO_SAMPLE *win = al_load_sample("win.wav");
   ALLEGRO_SAMPLE *fail = al_load_sample("fail.wav");
   ALLEGRO_SAMPLE *fundo = al_load_sample("fundo.wav");

   al_reserve_samples(4);

   BG.x = 0;
   BG.y = 0;
   BG.velX = 1;
   BG.velY = 0;
   BG.largura = L;
   BG.altura = A;
   BG.dirX = -1;
   BG.dirY = 1;
   BG.image = bgImage;

   MG.x = 0;
   MG.y = 0;
   MG.velX = 3;
   MG.velY = 0;
   MG.largura = L;
   MG.altura = A;
   MG.dirX = -1;
   MG.dirY = 1;
   MG.image = mgImage;

   // REGISTRA NA FILA DE EVENTOS
   al_register_event_source(fila_eventos, al_get_mouse_event_source());

   al_register_event_source(fila_eventos, al_get_display_event_source(tela));
   al_register_event_source(fila_eventos, al_get_timer_event_source(temporizador));

   al_flip_display();
   // INICIAR TEMPORIZADOR
   al_start_timer(temporizador);
   ALLEGRO_FONT *padrao = al_load_font("arial.ttf", 30, 0);
   al_play_sample(inicio, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);

   while (1)
   {
      al_play_sample(fundo, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
      ALLEGRO_EVENT ev;
      al_wait_for_event(fila_eventos, &ev);

      if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
      {
         if (principal.posicao_addx == 0)
            principal.posicao_addx = 0.00001;
         if (principal.posicao_addy == 0)
            principal.posicao_addy = 0.00001;

         principal.raio = principal.raio * 0.98;

         if ((principal.posicao_addx > 0 && (ev.mouse.x - principal.posicao_x) > 0))
         {
            principal.posicao_addx -= impulso_expelidas;
         }
         if ((principal.posicao_addx < 0 && (ev.mouse.x - principal.posicao_x) < 0))
         {
            principal.posicao_addx += impulso_expelidas;
         }
         if ((principal.posicao_addx > 0 && (ev.mouse.x - principal.posicao_x) < 0))
         {
            principal.posicao_addx += impulso_expelidas;
         }
         if ((principal.posicao_addx < 0 && (ev.mouse.x - principal.posicao_x) > 0))
         {
            principal.posicao_addx -= impulso_expelidas;
         }

         if ((principal.posicao_addy > 0 && (ev.mouse.y - principal.posicao_y) > 0))
         {
            principal.posicao_addy -= impulso_expelidas;
         }
         if ((principal.posicao_addy < 0 && (ev.mouse.y - principal.posicao_y) < 0))
         {
            principal.posicao_addy += impulso_expelidas;
         }
         if ((principal.posicao_addy > 0 && (ev.mouse.y - principal.posicao_y) < 0))
         {
            principal.posicao_addy += impulso_expelidas;
         }
         if ((principal.posicao_addy < 0 && (ev.mouse.y - principal.posicao_y) > 0))
         {
            principal.posicao_addy -= impulso_expelidas;
         }

         // EXPELE BOLINHAS

         N_BOLAS_SHOWED++;

         bola[N_BOLAS_SHOWED - 1] = cria_bago_aleatorio(1);

         bola[N_BOLAS_SHOWED - 1]->raio = principal.raio * 0.1;
         bola[N_BOLAS_SHOWED - 1]->posicao_addx = abs(principal.posicao_addx) * 2 * ((ev.mouse.x - principal.posicao_x) / dist(ev.mouse.x, principal.posicao_x, ev.mouse.y, principal.posicao_y));
         bola[N_BOLAS_SHOWED - 1]->posicao_addy = abs(principal.posicao_addy) * 2 * (ev.mouse.y - principal.posicao_y) / dist(ev.mouse.x, principal.posicao_x, ev.mouse.y, principal.posicao_y);
         bola[N_BOLAS_SHOWED - 1]->posicao_x = (principal.posicao_x + ev.mouse.x) / 2;
         bola[N_BOLAS_SHOWED - 1]->posicao_y = (principal.posicao_y + ev.mouse.y) / 2;
      }

      for (i = 0; i < N_BOLAS_SHOWED; i++)
      {
         if (bola[i]->raio != 0)
         {

            if (abs((calcular_distancia_bolas(principal, bola[i])) < (principal.raio + bola[i]->raio)) && principal.raio > (bola[i]->raio))
            {

               principal.raio = sqrt(pow(principal.raio, 2) + pow(bola[i]->raio, 2));
               bola[i]->raio = 0;
            }
            if (abs((calcular_distancia_bolas(principal, bola[i])) < (principal.raio + bola[i]->raio)) && principal.raio < (bola[i]->raio))
            {
               bola[i]->raio = sqrt(pow(principal.raio, 2) + pow(bola[i]->raio, 2));
               principal.raio = 0;
            }
         }
      }

      for (h = 0; h < N_BOLAS_SHOWED; h++)
      {

         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {

            if (bola[i]->raio != 0)
            {

               if (abs((calcular_distancia_bolas_2(bola[i], bola[h])) < (bola[i]->raio + bola[h]->raio)) && (bola[i]->raio) > (bola[h]->raio))
               {

                  bola[i]->raio = sqrt(pow(bola[h]->raio, 2) + pow(bola[i]->raio, 2));
                  bola[h]->raio = 0;
               }
               if (abs((calcular_distancia_bolas_2(bola[i], bola[h])) < (bola[i]->raio + bola[h]->raio)) && (bola[i]->raio) < (bola[h]->raio))
               {
                  bola[h]->raio = sqrt(pow(bola[h]->raio, 2) + pow(bola[i]->raio, 2));
                  bola[i]->raio = 0;
               }
            }
         }
      }

      if (ev.type == ALLEGRO_EVENT_TIMER)
      {

         // CONTADOR DE SCORE
         score = score + 0.1;

         // GERA NUMERO ALEATORIO
         aux_bola_aleatoria = rand() % 100;

         // ATUALIZA O FUNDO//

         BG.x = BG.x + BG.velX * BG.dirX;
         if (BG.x + BG.largura <= 0)
            BG.x = 0;

         MG.x = MG.x + MG.velX * MG.dirX;
         if (MG.x + MG.largura <= 0)
            MG.x = 0;

         // ATUALIZA POSIÇÃO DOS BAGOS//

         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {
            if (bola[i]->raio != 0)
            {

               if ((bola[i])->posicao_x < bola[i]->raio || (bola[i])->posicao_x > L - bola[i]->raio)
               {
                  // altera a direcao na qual o bouncer se move no eixo x
                  (bola[i])->posicao_addx = -((bola[i])->posicao_addx);
               }
            }
         }

         // verifica se a posicao y do bouncer passou dos limites da tela
         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {
            if (bola[i]->raio != 0)
            {

               if ((bola[i])->posicao_y < (bola[i])->raio || (bola[i])->posicao_y > A - (bola[i])->raio)
               {
                  // altera a direcao na qual o bouncer se move no eixo y
                  (bola[i])->posicao_addy = -((bola[i])->posicao_addy);
               }
            }
         }

         if (principal.posicao_y < principal.raio || principal.posicao_y > A - principal.raio)
         {
            // altera a direcao na qual o bouncer se move no eixo y
            principal.posicao_addy = -principal.posicao_addy;
         }

         if (principal.posicao_x < principal.raio || principal.posicao_x > (L - principal.raio))
         {
            // altera a direcao na qual o bouncer se move no eixo y
            principal.posicao_addx = -principal.posicao_addx;
         }

         // faz o bouncer se mover no eixo x e y incrementando as suas posicoes de bouncer_dx e bouncer_dy, respectivamente
         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {
            if (bola[i]->raio != 0)
            {
               (bola[i])->posicao_x += (bola[i])->posicao_addx;
               (bola[i])->posicao_y += (bola[i])->posicao_addy;
            }
         }

         principal.posicao_x += principal.posicao_addx;
         principal.posicao_y += principal.posicao_addy;

         redesenhar = true;

         if (aux_bola_aleatoria == 2)
         {
            N_BOLAS_SHOWED++;
            bola[N_BOLAS_SHOWED - 1] = cria_bago_aleatorio(1);
         }
      }

      // se o tipo de evento for o fechamento da tela (clique no x da janela)
      else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
      {

         al_destroy_timer(temporizador);
         al_destroy_display(tela);
         al_destroy_event_queue(fila_eventos);
         al_destroy_bitmap(MG.image);
         al_destroy_bitmap(BG.image);

         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {
            free(bola[i]);
         }

         return 0;
      }

      // se eu alterei a posicao do bouncer, o redraw foi para true e eu nao tenho eventos na fila para ler
      if (redesenhar && al_is_event_queue_empty(fila_eventos))
      {

         redesenhar = false;
         // limpo a tela
         al_clear_to_color(al_map_rgb(0, 0, 10));

         // DESENHA O FUNDO

         al_draw_bitmap(BG.image, BG.x, BG.y, 0);
         al_draw_bitmap(MG.image, MG.x, MG.y, 0);
         al_draw_bitmap(sc, 0, 0, 0);

         if (BG.x + BG.largura < L)
            al_draw_bitmap(BG.image, BG.x + BG.largura, BG.y, 0);
         if (MG.x + MG.largura < L)
            al_draw_bitmap(MG.image, MG.x + MG.largura, MG.y, 0);

         // DESENHA OS BAGOS
         for (i = 0; i < N_BOLAS_SHOWED; i++)
         {
            al_draw_filled_circle((bola[i])->posicao_x, (bola[i])->posicao_y, (bola[i])->raio, al_map_rgb((bola[i])->red, (bola[i])->gre, (bola[i])->blu));
         }

         // DESENHA A BOLA PRINCIPAL
         al_draw_filled_circle(principal.posicao_x, principal.posicao_y, principal.raio, al_map_rgb(principal.red, principal.gre, principal.blu));

         score_final = (float)1000000000 / score;
         al_draw_textf(padrao, al_map_rgb(0, 0, 0), 10, 5, ALLEGRO_ALIGN_LEFT, "Score: %.1f", score_final);

         // reinicializo a tela
         al_flip_display();

         if (principal.raio == 0)
         {
            resultado = false;
            al_rest(1.0);
            break;
         }
      }
      // CONFERE SE COMEU A INIMIGA
      if (bola[inimiga]->raio == 0)
      {
         resultado = true;
         al_rest(1.0);
         break;
      }

   } // fim do while

   // VERIFICA SE GANHOU OU PERDEU
   if (!resultado)
   {
      al_destroy_sample(fundo);
      al_play_sample(fail, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
      ALLEGRO_FONT *padrao_2 = al_load_font("arial.ttf", 50, 0);

      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_text(padrao, al_map_rgb(255, 255, 255), L / 2, A / 2, ALLEGRO_ALIGN_CENTRE, "VOCÊ PERDEU!!!");
      al_flip_display();
      al_rest(8.0);
      al_destroy_font(padrao_2);
   }

   if (resultado)
   {
      al_destroy_sample(fundo);
      al_play_sample(win, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
      float high_score;
      fscanf(log, "%f", &high_score);

      if (score_final > high_score)
      {
         FILE *log = fopen("score_log.txt", "w");
         fprintf(log, "%.1f", score_final);
         fclose(log);
      }

      ALLEGRO_FONT *padrao_2 = al_load_font("arial.ttf", 50, 0);
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_text(padrao, al_map_rgb(0, 255, 0), L / 2, A / 2 - 25, ALLEGRO_ALIGN_CENTRE, "GANHOU!!");
      al_draw_textf(padrao, al_map_rgb(255, 255, 255), L / 2, A / 2 + 30, ALLEGRO_ALIGN_CENTRE, "SCORE: %.1f", score_final);
      al_draw_textf(padrao, al_map_rgb(255, 255, 255), L / 2, A / 2 + 70, ALLEGRO_ALIGN_CENTRE, "ULTIMO RECORDE: %.1f", high_score);
      al_flip_display();
      al_rest(8.0);
      al_destroy_font(padrao_2);
      fclose(log);
   }

   // procedimentos de fim de jogo (fecha a tela, limpa a memoria, etc)

   al_destroy_timer(temporizador);
   al_destroy_display(tela);
   al_destroy_event_queue(fila_eventos);
   al_destroy_bitmap(MG.image);
   al_destroy_bitmap(BG.image);
   al_destroy_bitmap(sc);
   al_destroy_sample(win);
   al_destroy_sample(fail);
   al_destroy_sample(inicio);

   for (i = 0; i < N_BOLAS_SHOWED; i++)
   {
      free(bola[i]);
   }

   return 0;
}
