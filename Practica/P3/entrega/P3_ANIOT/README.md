# Practica 3 -- Programación con tareas y eventos en ESP-IDF

## Cuestión 1 — Tarea muestreadora / app_main

### ¿Qué prioridad tiene la tarea inicial que ejecuta la función app_main()?

La función app_main tiene por defecto prioridad '1' según la documentación de Espressif.
[Referencia:](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#background-tasks)

En nuestro proyecto, la prioridad de las tareas creadas es '5'.

### ¿Con qué llamada de ESP-IDF podemos conocer la prioridad de una tarea?

Con la llamada uxTaskPriorityGet. Este retorna la prioridad de la tarea dada.

## Cuestión 2 — Sincronización entre tareas

### ¿Cómo sincronizas ambas tareas (la muestreadora y la inicial)?

En nuestro proyecto se sincronizan por medio de colas(xQueue) y eventos(esp_event). 
Una de las tareas que actúa según los eventos alimenta de datos a la otra por medio de una cola.

### ¿Cómo sabe la tarea inicial que hay un nuevo dato generado por la tarea muestreadora?

La tarea muestreadora genera un nuevo evento cada vez que recibe un dato. 
Este evento es depositado en una cola que es observada por la máquina de estados. Mientras no se alimente de eventos a esta cola, la máquina de estados se detiene.

## Cuestión 3 — Pasar múltiples argumentos a una tarea

### Si además de pasar el período como parámetro, quisiéramos pasar como argumento la dirección en la que la tarea muestreadora debe escribir las lecturas, ¿cómo pasaríamos los dos argumentos a la nueva tarea?

El argumento que acepta xTaskCreate es de tipo (void *). Podría ser usado para pasar la información necesaria a la tarea por medio de un struct.

## Cuestión 4 — Comunicación mediante colas

### Al enviar un dato por una cola, ¿el dato se pasa por copia o por referencia?
(Consulta la documentación para responder.)

Según la documentación, el dato es copiado.

[Referencia:]([https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#background-tasks](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#c.xQueueSendToBack))


## Cuestión 5 — Uso de eventos

### ¿Qué debe hacer la tarea inicial tras registrar el handler? ¿Puede finalizar?

La tarea no puede finalizar. Debe mantenerse activa para poder tratar los eventos. Tras registrar un evento debe esperar a que este ocurra.
