#include "friendmodel.hpp"
#include "db.hpp"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除好友关系
void FriendModel::delete_friend(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    // DELETE FROM friend
    // WHERE (userid = 3 AND friendid = 4)
    sprintf(sql, "delete from friend where (userid = %d and friendid = %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}


// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};

    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);

    // 所有好友的信息
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的用户放回 判断有无新好友
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}