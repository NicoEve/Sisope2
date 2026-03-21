# Simulador MLFQ (Multi-Level Feedback Queue)

Este proyecto implementa un simulador de planificación de procesos basado en el algoritmo MLFQ (Multi-Level Feedback Queue) en C++. El programa permite ejecutar diferentes configuraciones de colas y analizar métricas como tiempo de espera, turnaround, response, entre otros.

# Descripción

El algoritmo MLFQ organiza los procesos en múltiples colas con diferentes niveles de prioridad:
  -La cola 1 tiene mayor prioridad.
  -La cola 4 tiene menor prioridad.
  -Siempre se ejecuta primero la cola más prioritaria que tenga procesos.
Cada cola puede usar un algoritmo diferente:
  -RR (Round Robin) → con quantum
  -SJF (Shortest Job First) → no expropiativo
  -STCF (Shortest Time to Completion First) → expropiativo
Además:
Si llega un proceso en una cola más prioritaria, puede interrumpir la ejecución actual.
Dentro de cada cola se respeta la prioridad y orden de llegada.

# Esquemas implementados

El simulador soporta 3 configuraciones:
### Esquema 1
RR(1), RR(3), RR(4), SJF
### Esquema 2
RR(2), RR(3), RR(4), STCF
### Esquema 3
RR(3), RR(5), RR(6), RR(20)

# Formato de entrada

Archivo .txt con el siguiente formato:
# label; BT; AT; Q; Priority
-A;6;0;1;5
-B;9;0;1;4
-C;10;0;2;3
-D;15;0;2;3
-E;8;0;3;2
-F;7;2;4;1
Campos:
-label → nombre del proceso
-BT → Burst Time
-AT → Arrival Time
-Q → Cola (1 a 4)
-Priority → prioridad dentro de la cola

# Ejecución

Compilar:
-g++ -o mlfq main.cpp
Ejecutar:
-./mlfq archivoEntrada.txt esquema archivoSalida.txt
Ejemplo:
-./mlfq mlq001.txt 1 output.txt

# Salida

El programa genera:
✔ Consola
Tabla con:
  -WT → Waiting Time
  -CT → Completion Time
  -RT → Response Time
  -TAT → Turnaround Time
✔ Archivo de salida
Formato:
# etiqueta; BT; AT; Q; Pr; WT; CT; RT; TAT
-A;6;0;1;5;...;...;...;...
-WT=...; CT=...; RT=...; TAT=...;
También incluye:
  -Línea de tiempo de ejecución (timeline)

# Lógica del simulador
Se leen los procesos desde el archivo.
Se organizan en 4 colas según su nivel.
En cada instante:
  -Se agregan los procesos que han llegado.
  -Se selecciona la cola de mayor prioridad disponible.
  -Se ejecuta según su política (RR, SJF o STCF).
  -Se actualizan métricas al finalizar cada proceso.
