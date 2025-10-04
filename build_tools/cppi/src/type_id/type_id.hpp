#pragma once


class type_id_2 {
    const type_id_2* next = nullptr;

public:
    virtual ~type_id_2() {}

};


class type_id_2_pointer : public type_id_2 {
public:
};

class type_id_2_lref : public type_id_2 {
public:
};

class type_id_2_rref : public type_id_2 {
public:
};

class type_id_2_array : public type_id_2 {
public:
};

class type_id_2_func : public type_id_2 {
public:
};

