c++0x/11/14 adapter -- std
=============

foreach的实现参考了BOOST_FOREACH，肯定没它的在不同编译器上的兼容性好

如果项目使用了Boost还是建议定义 OWENT_WITH_BOOST_HPP 和 OWENT_ENABLE_BOOST_FOREACH 宏来转向Boost里的foreach函数

如果有任何Bug欢迎 mailto: owt5008137@live.com
