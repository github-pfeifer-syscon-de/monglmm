
in vec4 vertexColor;
in vec4 gamma;
out vec4 fragmentColor;
void main() {
    fragmentColor = pow(vertexColor, gamma);

}
