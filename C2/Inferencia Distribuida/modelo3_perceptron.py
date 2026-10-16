################################################################################
# model3 - Perceptrón multiclase (grupo de Almeida, Adrían, Jesús H y Cesar)

import numpy as np

_INVERTIR_SENSOR = True #se el valor para no cambiar los datos de entrenamiento, porque está pensado en lo más bajo como el color determinante


class PerceptronMulticlase:

    def __init__(self, numero_entradas, numero_clases, tasa_aprendizaje=0.1):
        self.numero_entradas = numero_entradas
        self.numero_clases = numero_clases
        self.tasa_aprendizaje = tasa_aprendizaje
        self.pesos = np.zeros((numero_clases, numero_entradas))
        self.bias = np.zeros(numero_clases)

    def entrenar(self, X, y, epocas=100, verbose=False):
        for epoca in range(epocas):
            errores = 0

            for entrada, clase_real in zip(X, y):
                activaciones = np.dot(self.pesos, entrada) + self.bias
                clase_predicha = np.argmax(activaciones)

                if clase_predicha != clase_real:
                    self.pesos[clase_real] += self.tasa_aprendizaje * entrada
                    self.bias[clase_real] += self.tasa_aprendizaje
                    self.pesos[clase_predicha] -= self.tasa_aprendizaje * entrada
                    self.bias[clase_predicha] -= self.tasa_aprendizaje
                    errores += 1

            if verbose:
                print(f"Época {epoca + 1:03d} | Errores: {errores}")

            if errores == 0:
                if verbose:
                    print(f"Perceptrón entrenado en {epoca + 1} épocas.")
                break

    def predecir(self, entrada):
        activaciones = np.dot(self.pesos, entrada) + self.bias
        return int(np.argmax(activaciones))


# Datos de entrenamiento normalizados (0 a 1): [R, G, B]
# Clase 0 = Rojo, 1 = Verde, 2 = Azul
_X = np.array([
    # ROJO
    [1.00, 0.05, 0.05], [0.95, 0.10, 0.05], [0.90, 0.05, 0.10],
    [0.85, 0.10, 0.05], [0.90, 0.15, 0.10], [0.80, 0.05, 0.05],
    # VERDE
    [0.05, 1.00, 0.05], [0.10, 0.95, 0.05], [0.05, 0.90, 0.10],
    [0.10, 0.85, 0.05], [0.15, 0.90, 0.10], [0.05, 0.80, 0.05],
    # AZUL
    [0.05, 0.05, 1.00], [0.05, 0.10, 0.95], [0.10, 0.05, 0.90],
    [0.05, 0.15, 0.90], [0.10, 0.10, 0.85], [0.05, 0.05, 0.80]
])

_Y = np.array([0]*6 + [1]*6 + [2]*6)

_COLORES = {0: "Rojo", 1: "Verde", 2: "Azul"}

# Se entrena UNA sola vez, cuando se importa este archivo (al iniciar Flask)
_modelo = PerceptronMulticlase(numero_entradas=3, numero_clases=3, tasa_aprendizaje=0.1)
_modelo.entrenar(_X, _Y, epocas=100)


def model3(R, G, B):
    """Recibe R, G, B (0-255) y retorna 'Rojo', 'Verde' o 'Azul'."""
    valores = np.array([float(R), float(G), float(B)])
    if _INVERTIR_SENSOR:
        valores = 255 - np.clip(valores, 0, 255)
    entrada = valores / 255.0
    return _COLORES[_modelo.predecir(entrada)]


if __name__ == "__main__":
    # Prueba rápida solo si se ejecuta este archivo directamente
    print(model3(240, 25, 20))  # Rojo