# hierashards 

## 安装方法

+ git clone https://github.com/JasonXQH/hierashards.git
+ mkdir build&&cd build
+ 运行sudo cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .. -G 'Unix Makefiles'
+ 运行 cmake ..
+ 运行 make -j12

## 第二阶段 实现静态层面的打包节点的切换

已经设置全局变量 sealingNodes,并设置默认参数为4

初步思考需要修改的函数

在reqNum=5的时候, 随即选择两个节点作为5-10轮的打包节点,我这里暂定0,1作为打包节点,此时,广播出去的prepareReq=2, Cache中只有两个Prepare信息dyveb



在reqNum=10的时候,随机选择三个节点作为10-15轮

