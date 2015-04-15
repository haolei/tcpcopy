**English document**

Please visit https://github.com/session-replay-tools/tcpcopy

**Chinese Description**

**注意最新公告**
```
1）由于google不再提供下载，如需要采用最新版本，请访问如下地址
https://github.com/session-replay-tools/tcpcopy
```

```
2）这里只维护中文版本（0.9.6版本为止）

3）0.5以下版本使用方法参考百度百科：
```

http://baike.baidu.com/view/9296140.htm

```
4）高级用法和问题诊断参考最新文档：
```
http://tcpcopy.googlecode.com/files/TCPCopy_Manual_v0.9.6%28Chinese%29.pdf.pdf

```
5）各种会议的文档：
```
https://github.com/session-replay-tools/tcpcopy/downloads

```
7）如果遇到问题，请先到github issues版面进行查找，看看有没有类似的问题
```
https://github.com/session-replay-tools/tcpcopy/issues

```
8）QQ交流群：192300573 or 330100216，邮箱:wangbin579@gmail.com
```


```
9）以下内容只针对tcpcopy 0.5～0.9.6
```


**简介**
```
TCPCopy是一种请求复制（所有基于tcp的packets）工具，可以把在线请求导入到测试系统中去。
我们曾经应用于网易的广告投放系统，urs系统，nginx hmux协议等系统，避免了上线带来的很多问题。

目前此工具已经广泛应用于国内各大互联网公司。
```


**TCPCopy七大功能：**
```
1）分布式压力测试工具，利用在线数据，可以测试系统能够承受的压力大小（远比ab压力测
试工具真实地多）,也可以提前发现一些bug
2）普通上线测试，可以发现新系统是否稳定，提前发现上线过程中会出现的诸多问题，让开
发者有信心上线
3）对比试验，同样请求，针对不同或不同版本程序，可以做性能对比等试验
4）流量放大功能
5）利用TCPCopy转发传统压力测试工具发出的请求，可以增加网络延迟，使其压力测试更加真实
6）热备份
7）实战演习（架构师必备）
```


**特点：**
```
1）实时 （离线通过configure --enable-offline)
2）效果真实
3）低负载，不影响在线
4）操作简单
5）分布式
6）零成本
```


**使用方法：**
```
TCPCopy分为TCPCopy client(即tcpcopy)和TCPCopy server(即intercept)。其中TCPCopy client
运行在在线服务器上面，用来捕获在线请求数据包；TCPCopy server（默认监听端口为36524）运行在
测试机器上面，用来截获响应包，并传递响应包头信息给TCPCopy client,以完成TCP交互。

TCPCopy使用分为传统使用方式和高级使用方式，具体原理见：
http://blog.csdn.net/wangbin579/article/details/8949315


传统使用方法：
TCPCopy server （root用户执行）
     采用IP Queue 模块（内核<3.5，默认采用IP Queue）:
      1）启动内核模块ip_queue (modprobe ip_queue)
         （如果已经启动ip_queue，则不需要此步骤）
      2）设置要截获的端口，并且设置对output截获
          iptables -I OUTPUT -p tcp --sport port -j QUEUE
         （如果已经设置上述命令，则不需要此步骤）
      3）./intercept

    或者采用NFQueue 模块（内核>=3.5，默认采用NFQueue）:
      1） iptables -I OUTPUT -p tcp --sport port -j NFQUEUE 
          （如果已经设置上述命令，则不需要此步骤）
      2） ./intercept


TCPCopy client （root用户执行）
      ./tcpcopy -x 服务器应用端口号-测试服务器ip地址:测试服务器应用端口



高级使用方式：
参考：
http://blog.csdn.net/wangbin579/article/details/8950282
http://blog.csdn.net/wangbin579/article/details/8994601
http://blog.csdn.net/wangbin579/article/details/10148247

```

**举例(传统使用方式）**
```
假设1.2.3.25，1.2.3.26是在线应用服务器，1.2.3.161是测试服务器，在线应用端口是11311,测试
服务器的应用端口都是11511,我们的目的就是为了确认1.2.3.161测试服务器能否承受目前两台在线的压力。
我们利用TCPCopy进行测试：
    目标测试服务器(1.2.3.161，内核2.6.18，采用IP Queue来捕获响应包)
       # modprobe ip_queue 
       # iptables -I OUTPUT -p tcp --sport 11511 -j QUEUE 
       # ./intercept
    在线服务器(1.2.3.25):
       # ./tcpcopy -x 11311-1.2.3.161:11511
    在线服务器(1.2.3.26):
       # ./tcpcopy -x 11311-1.2.3.161:11511

       1.2.3.25:
           21158 appuser   15   0  271m 226m  756 S 24.2  0.9  16410:57 asyn_server
           9168  root      15   0 18436  12m  380 S  8.9  0.1  40:59.15 tcpcopy
       1.2.3.26:
           16708 appuser   15   0  268m 225m  756 S 25.8  0.9  17066:19 asyn_server
           11662 root      15   0 17048  10m  372 S  9.3  0.0  53:51.49 tcpcopy
       1.2.3.161:
           27954 root      15   0  284m  57m  828 S 58.6  1.4 409:18.94 asyn_server
           1476  root      15   0 14784  11m  308 S  7.7  0.3  49:36.93 intercept
       1.2.3.25:
           $ wc -l access_1109_09.log
             7867867,  2185 reqs/sec
       1.2.3.26:
           $ wc -l access_1109_09.log
             7843259,  2178 reqs/sec
       1.2.3.161:
           $ wc -l access_1109_09.log
             15705229, 4362 reqs/sec

请求丢失率为：(7867867 + 7843259 - 15705229) / (7867867 + 7843259) = 0.0375%
从上面可以看出1.2.3.161测试服务器能够承受目前在线压力的两倍。
我们来看负载情况：
tcpcopy自身负载占到8.9%和9.3%，intercept占到7.7%，从负载来看，均不高，内存也占得不多

```

**传统使用方式注意事项(以下只针对0.5~0.9.6，老版本参考http://baike.baidu.com/view/9296140.htm)*
```
（源代码转移到了github，敬请注意）

1）Linux平台，内核2.6+，目前tcpcopy传统架构需要支持netlink机制或者nfqueue(0.6.5版本+支持
nfqueue,在./configure指定nfqueue即可或者对于0.7.0+版本，如果内核为3.5+,则自动采用nfqueue模式)

2）TCPCopy中的tcpcopy和intercept程序运行需要root权限

3）intercept在同一台机器只需要运行一个实例就能支持多个应用的复制（设置多条iptables命令）

4）TCPCopy client需要连接测试服务器（默认36524端口），所以要对外开放相应端口

5）TCPCopy由于依赖于抓包函数，压力大的时候，抓包函数本身不可靠，所以会丢包，进而丢失请求

6）如果采用的是IP Queue模块来截获响应包，则intercept程序密切跟ip queue内核模块相关，
所以当压力很大的时候请求丢失率很高，需要优化sysctl系统参数才能达到好的效果
（通过cat /proc/net/ip_queue，查看ip queue运行情况，如果Queue dropped的数值不断增大，
则需要修改ip_queue_maxlen参数，比如echo 4096 > /proc/sys/net/ipv4/ip_queue_maxlen；
如果Netlink dropped的数值不断增大，修改net.core.rmem_max和net.core.wmem_max参数,
比如sysctl -w net.core.rmem_max=16777216和sysctl -w net.core.wmem_max=16777216）

7）如果要复制127.0.0.1发出的请求到另外一台机器，需要设置-c参数

8）测试环境最好和在线环境一致，比如连接都保持keepalive

9）TCPCopy只与ip、tcp层的数据有关，如果请求验证与tcp层以上的协议有关，则系统不能正常运行。
例如：mysql连接协议，由于权限认证与tcp层上面的mysql协议有关，所以复制过去的请求会被目标测试
服务器认为非法请求，这个时候需要针对mysql协议作具体针对性的处理，tcpcopy程序才能正常运行

10）多层架构环境下，测试系统一定要独立，与在线系统没有业务关联，否则会影响在线

11）丢失请求率跟网络状况有关，最好在内网内复制请求

12）本系统不支持域名，只支持ip地址

13）更多信息参考如下文档
(http://tcpcopy.googlecode.com/files/TCPCopy_Manual_v0.9.6%28Chinese%29.pdf.pdf)
```**

**总结**
```
如果你对上线没有信心，如果你的单元测试不够充分，如果你对新系统不够有把握，如果你对未来的请求
压力无法预测，如果你想对比诸如apache和nginx的性能，如果你想放大在线流量，TCPCopy可以帮助
你解决上述难题
```

**感谢**
```
感谢网易丁磊的支持,感谢网易技术部的支持,感谢原先作者王波的设计和部分代码,感谢淘宝开源人士的支
持(比如淘叔度,一啸的支持),感谢叶金荣的mysql测试支持,感谢众多开源人士的支持.
```