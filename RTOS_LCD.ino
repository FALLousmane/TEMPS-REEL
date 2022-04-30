//:::::::::::::::::::::::::::::::::::::OUSMANE FALL::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::::::::::::::::::::::::::::::::::::SUP GALILEE:::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::::::::::::::::::::::::::::::::::::TEMPS REEL::::::::::::::::::::::::::::::::::::::::::::::::::::::


//LIBRAIRIES A UTILISER
#include <LiquidCrystal_I2C.h> 
#include <Arduino_FreeRTOS.h>  
#include "queue.h"
#include "semphr.h"
#include "task.h"

//LES PORTS A UTILISER

#define BP1 3     // 1er  BOUTON POUSSOIR
#define BP2 4    //  2éme BOUTON POUSSOIR
#define POT A0  //   POTENTIOMETRE

LiquidCrystal_I2C lcd(0x27,16,2); // ECRAN LCD_I2C 2 LIGNES ET A6 COLONNES AVEC L'ADRESSE 0x27


// PRIORITES DES TACHES
#define PRIORITY_TASK_1  (tskIDLE_PRIORITY+1) 
#define PRIORITY_TASK_2  (tskIDLE_PRIORITY+1) 
#define PRIORITY_TASK_3  (tskIDLE_PRIORITY+2)
#define PRIORITY_TASK_4  (tskIDLE_PRIORITY+2) 
#define PRIORITY_TASK_5  (tskIDLE_PRIORITY+3)

//QUEUES
QueueHandle_t SendToTask3_1;
QueueHandle_t SendToTask3_2;
QueueHandle_t SendToTask4;
QueueHandle_t SendToTask5;

//SEMAPHORE
SemaphoreHandle_t sem; 

//LA STRUCTURE REGROUPANT LES DONNEES
typedef struct Donnees
{
    int POTENTIO;
    int RESULTAT;
    double TEMPS;
}Donnees;


void setup()
{
  
  lcd.init();   // INITIALISATION DE L'ECRAN LCD_I2C                  
  lcd.backlight(); // ALLUMER L'ECLAIRAGE DE L'ECRAN
  
  Serial.begin(9600);       //COMMUNICATION SERIE à 9600 Bauds

  // MESSAGE DE BIENVENUE
  lcd.begin(16, 2);
  lcd.print(" BIENVENUE AU ");
  lcd.setCursor(0,1);   
  lcd.print(" TP-PROJET RTOS ");
  delay(1000);
  lcd.clear(); // EFFACER LES ECRITURES POUR PREPARER UNE NOUVELLE ECRITURE
  

  //DEFINITION DES ENTREES
  pinMode(BP1, INPUT);  
  pinMode(BP2, INPUT);
  pinMode(POT, INPUT);

  //Initialisation du sémaphore
  sem = xSemaphoreCreateBinary();

  //Création des queues
  SendToTask3_1 = xQueueCreate(1, sizeof(int)); 
  SendToTask3_2 = xQueueCreate(1, sizeof(int)); 
  SendToTask4 = xQueueCreate(1, sizeof(Donnees)); 
  SendToTask5 = xQueueCreate(1, sizeof(Donnees)); 
  
  //Création des tâches
  xTaskCreate(tache1, "tache1", 1000, NULL, PRIORITY_TASK_1, NULL);   //Tâche 1
  xTaskCreate(tache2, "tache2", 1000, NULL, PRIORITY_TASK_2, NULL);   //Tâche 2
  xTaskCreate(tache3, "tache3", 1000, NULL, PRIORITY_TASK_3, NULL);   //Tâche 3
  xTaskCreate(tache4, "tache4", 1000, NULL, PRIORITY_TASK_4, NULL);   //Tâche 4
  xTaskCreate(tache5, "tache5", 1000, NULL, PRIORITY_TASK_5, NULL);   //Tâche 5

  //Appel de l'ordonnanceur
  vTaskStartScheduler(); 
}    

void tache1(void* pvParameters) //LIRE LA VALEUR DU POTENTIO ET L'ENVOI A LA TACHE 3
{
    while (1)
  {
    int VALEUR = analogRead(POT);

    xQueueSend(SendToTask3_1, &VALEUR, portMAX_DELAY); //FILE D'ATENTE
  }
}

void tache2(void* pvParameters) //RESULTAT DE LA SOMME ET ENVOIE VERS A LA TACHE3
{
  while(1)
  {
    int ETAT1 = digitalRead(BP1);
    int ETAT2 = digitalRead(BP2);
    int SOMME = ETAT1 + ETAT2;

    xQueueSend(SendToTask3_2, &SOMME, portMAX_DELAY); //File pour envoyer à la tâche 3
  }
}

void tache3(void* pvParameters) //RECEVOIR LES DONNEES DES TACHES PRECEDENTES
{
  while(1)
  {
    double startTime = millis();

    Donnees structValeurs;
    int receivedAnalogValue;
    int receivedDigitalValue;

    xQueueReceive(SendToTask3_1, &receivedAnalogValue, portMAX_DELAY); 
    xQueueReceive(SendToTask3_2, &receivedDigitalValue, portMAX_DELAY); 

    structValeurs.POTENTIO = receivedAnalogValue;
    structValeurs.RESULTAT = receivedDigitalValue;
    structValeurs.TEMPS = startTime;

    xQueueSend(SendToTask4, &structValeurs, portMAX_DELAY); //Envoi de la structure à la tâche 4
  }
}

void tache4(void* pvParameters)   //AFFICHAGE DE LA STRUCTURE ET L'ENVOIE A LA TACHE 5
{
  while(1)
  {
    Donnees structValeurs;
    xQueueReceive(SendToTask4, &structValeurs, portMAX_DELAY); 

     // AFFICHAGE SUR L'ECRAN LCD
    lcd.begin(16, 2);
    lcd.print(" AFFICHAGE DE LA ");
    lcd.setCursor(0,1);
    lcd.print(" TACHE 4 :::>>");
    delay(100);
    lcd.clear(); // EFFACER LES ECRITURES PRECEDENTES
    lcd.begin(16, 2);
    lcd.print("POT   SUM  T(S)");
    lcd.setCursor(0,1);
    lcd.print(structValeurs.POTENTIO);
    lcd.setCursor(7,1);
    lcd.print(structValeurs.RESULTAT);
    lcd.setCursor(11,1);
    lcd.print(structValeurs.TEMPS);

    //AFFICHAGE SUR LE PORT SERIE
    Serial.println("**********TACHE 4**********");
    Serial.print("POTENTIOMETRE : ");
    Serial.println(structValeurs.POTENTIO);
    Serial.print("SOMME : ");
    Serial.println(structValeurs.RESULTAT);
    Serial.print("TIME (ms) : ");
    Serial.println(structValeurs.TEMPS);

    xQueueSend(SendToTask5, &structValeurs, portMAX_DELAY); //ENVOI DE LA STRUCTURE A LA TACHE 5

    xSemaphoreGive(sem);  //BLOCAGE DE LA TACHE 5

    //ATTENTE
    vTaskDelay(50);
  }
}

void tache5(void* pvParameters)   
{
  while(1)
  {
    xSemaphoreTake(sem,portMAX_DELAY); 

    Donnees structValeurs;

    xQueueReceive(SendToTask5, &structValeurs, portMAX_DELAY); 

    //Conversion du temps (en ms) en minutes
    structValeurs.TEMPS = structValeurs.TEMPS*(0.001/60);

    // AFFICHAGE SUR L'ECRAN LCD
    lcd.begin(16, 2);
    lcd.print(" AFFICHAGE DE LA ");
    lcd.setCursor(0,1);
    lcd.print(" TACHE 4 :::>>");
    delay(100);
    lcd.clear(); // EFFACER LES ECRITURES PRECEDENTES
    lcd.begin(16, 2);
    lcd.print("POT   SUM  TIME");
    lcd.setCursor(0,1);
    lcd.print(structValeurs.POTENTIO);
    lcd.setCursor(7,1);
    lcd.print(structValeurs.RESULTAT);
    lcd.setCursor(11,1);
    lcd.print(structValeurs.TEMPS);
    
    //AFFICHAGE SUR LE PORT SERIE
    Serial.println("::::::::::::TACHE 5::::::::::::");
    Serial.print("POTENTIOMETRE : ");
    Serial.println(structValeurs.POTENTIO);
    Serial.print("SOMME : ");
    Serial.println(structValeurs.RESULTAT);
    Serial.print("TIME(en MIN) : ");
    Serial.println(structValeurs.TEMPS);
    
    
    //ATTENTE
    vTaskDelay(5);
  }
}


void loop()
{
  // ON NE FAIT RIEN ICI
}
