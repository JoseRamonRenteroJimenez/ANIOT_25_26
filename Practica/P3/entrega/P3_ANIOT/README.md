# Practica 3 -- Programación con tareas y eventos en ESP-IDF

## Cuestión 1 — Tarea muestreadora / app_main

### ¿Qué prioridad tiene la tarea inicial que ejecuta la función app_main()?

### ¿Con qué llamada de ESP-IDF podemos conocer la prioridad de una tarea?

## Cuestión 2 — Sincronización entre tareas

### ¿Cómo sincronizas ambas tareas (la muestreadora y la inicial)?

### ¿Cómo sabe la tarea inicial que hay un nuevo dato generado por la tarea muestreadora?

## Cuestión 3 — Pasar múltiples argumentos a una tarea

Si además de pasar el período como parámetro, quisiéramos pasar como argumento la dirección en la que la tarea muestreadora debe escribir las lecturas, ¿cómo pasaríamos los dos argumentos a la nueva tarea?

## Cuestión 4 — Comunicación mediante colas

### Al enviar un dato por una cola, ¿el dato se pasa por copia o por referencia?
(Consulta la documentación para responder.)

## Cuestión 5 — Uso de eventos

### ¿Qué debe hacer la tarea inicial tras registrar el handler? ¿Puede finalizar?