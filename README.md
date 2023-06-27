Hocam merhaba,

Projeyi 2021 Macbook Pro, Xcode Version 14.0.1 kullanarak develop ettim.

Çalıştırmak için gerekli stepler:

1. OpenGL Library'lerini indirmek. Terminal uzerinden homebrew kullanarak yapabilirsiniz.
 A. brew install glfw
 B. brew install glm
 C. brew install glew
 
2. Projeyi unzip'lemek, deferred-rendering-project/SetupOpenGLExample.csproj dosyasına double-click yapıp Xcode üzerinden açmak.

3. main.cpp içindeki GetPath() methoduna projenin absolute path'ini vermek (bende sadece absolute path'ler çalıştı, nedenini bilmiyorum:

string GetPath(string originalPath){
    // TODO: Replace this with your own project path
    return "/Users/bartubas/Homeworks/deferred-rendering-project/SetupOpenGLExample/" + originalPath;
    // return originalPath;
}

4. Xcode üzerinde sol üstten play'e basıp build alıp oynamak.

5. Hocam herhangi bir build sorunu olursa çok kısa bir youtube tutorial'ı var 5 dakikalık, ben tüm ayarları bunu kullanarak yaptım, Mac'te OpenGL ilk setup'unu yapması çok uğraştırabiliyor:

https://youtu.be/MHlbNbWlrIM


--- 

Gameplay

WASD - Walking

Mouse Scroll - Camera Panning

Left Mouse Click - Throwing Lights

P - Toggle Pause/Unpause simulation

F - Toggle Forward/Deferred rendering

Escape - Exit Program

Render Mode, Light Count and Render Time is printed on Console.
